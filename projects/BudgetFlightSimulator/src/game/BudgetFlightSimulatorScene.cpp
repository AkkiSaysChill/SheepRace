#include "game/BudgetFlightSimulatorScene.h"
#include "engine/DrawableComponent.h"
#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/PlayerProgress.h"
#include "game/AircraftDatabase.h"
#include "game/FlightSession.h"
#include "game/MissionDatabase.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

BudgetFlightSimulatorScene::BudgetFlightSimulatorScene() : m_engine(nullptr) {}

BudgetFlightSimulatorScene::~BudgetFlightSimulatorScene() { Shutdown(); }

void BudgetFlightSimulatorScene::SwitchAircraft(const std::string &planeId) {
  AircraftStats newStats;
  if (!AircraftDatabase::GetInstance().GetStats(planeId, newStats))
    return;

  m_aircraft.SetStats(newStats);

  auto it = m_loadedAircraftModels.find(newStats.modelFile);
  if (it != m_loadedAircraftModels.end()) {
    m_planeModel = it->second;
    m_modelLoaded = true;
  }

  BoundingBox modelBounds = GetModelBoundingBox(m_planeModel);
  m_flightCamera.AutoAdjustForModel(modelBounds);

  if (m_planeObject) {
    auto drawable = m_planeObject->GetComponent<DrawableComponent>();
    if (drawable) {
      drawable->GetModelRef() = m_planeModel;
    }
  }
}

void BudgetFlightSimulatorScene::TriggerExplosionFX(Vector3 pos,
                                                    float intensity) {
  if (m_explosionEmitter) {
    m_explosionEmitter->Emit((int)(70.0f * intensity), pos);
  }
  if (m_shockwaveEmitter) {
    m_shockwaveEmitter->Emit((int)(2.0f * intensity), pos);
  }
  if (m_runwaySmokeEmitter) {
    m_runwaySmokeEmitter->Emit((int)(40.0f * intensity), pos);
  }
  m_explosionPostFXTimer = 0.45f * intensity;
  m_screenShakeIntensity = 2.8f * intensity;
}

void BudgetFlightSimulatorScene::Initialize(Engine *engine) {
  Scene3D::Initialize(engine);
  m_engine = engine;

  // Ingest custom Audiowide font
  const char *fontPath = "DATA/FONTS/Audiowide-Regular.ttf";
  if (FileExists(fontPath)) {
    m_hudFont = LoadFontEx(fontPath, 36, 0, 0);
    SetTextureFilter(m_hudFont.texture, TEXTURE_FILTER_BILINEAR);
    m_customFontLoaded = true;
  } else if (FileExists("DATA/fonts/Audiowide-Regular.ttf")) {
    m_hudFont = LoadFontEx("DATA/fonts/Audiowide-Regular.ttf", 36, 0, 0);
    SetTextureFilter(m_hudFont.texture, TEXTURE_FILTER_BILINEAR);
    m_customFontLoaded = true;
  }

  m_terrain.LoadWorld(m_engine, "island_world.glb");
  m_ocean.Initialize(m_engine, 260000.0f);

  m_aircraft.Reset({-4850.0f, 111.5f, 150.0f});
  m_aircraft.isOnGround = true;
  m_throttle = 0.0f;

  m_flightCamera.Initialize(55.0f, 15.0f);

  auto &gameObjectManager = m_engine->GetGameObjectManager();
  auto &assetLoader = m_engine->GetAssetLoader();

  assetLoader.loadPackedFile("shaders.bin");
  assetLoader.loadPackedFile("planes.woosh");
  assetLoader.loadPackedFile("effects.dyum");
  assetLoader.loadPackedFile("levelDAT");

  m_engine->PostProcessingActive = true;
  auto &postProc = m_engine->GetPostProcessing();
  postProc.SetShader(FX_CEL_FXAA);

  Shader fxaaShader = postProc.GetCurrentShader();
  Vector2 res = {(float)GetRenderWidth(), (float)GetRenderHeight()};
  int resLoc = GetShaderLocation(fxaaShader, "resolution");
  if (resLoc != -1) {
    SetShaderValue(fxaaShader, resLoc, &res, SHADER_UNIFORM_VEC2);
  }

  m_engine->GetDayNightCycle().SetTime(10.5f);
  m_engine->GetDayNightCycle().SetTimeScale(0.05f);

  auto xmlData = assetLoader.getAssetData("aircraft.xml");
  if (!xmlData.empty()) {
    AircraftDatabase::GetInstance().LoadFromMemory(xmlData.data(),
                                                   xmlData.size());
  } else {
    AircraftDatabase::GetInstance().LoadFromFile("assets/data/aircraft.xml");
  }

  // Pre-load and cache airframes
  Shader shadowShader = m_engine->GetRenderer().GetShadowShader();
  shadowShader.locs[SHADER_LOC_COLOR_DIFFUSE] =
      GetShaderLocation(shadowShader, "colDiffuse");
  shadowShader.locs[SHADER_LOC_MAP_ALBEDO] =
      GetShaderLocation(shadowShader, "texture0");

  const auto &ids = AircraftDatabase::GetInstance().GetAllPlaneIds();
  for (const auto &id : ids) {
    AircraftStats stats;
    if (AircraftDatabase::GetInstance().GetStats(id, stats)) {
      if (m_loadedAircraftModels.find(stats.modelFile) ==
          m_loadedAircraftModels.end()) {
        auto planeData = assetLoader.getAssetData(stats.modelFile);
        if (!planeData.empty()) {
          Model mdl = assetLoader.LoadModelFromAssetData(planeData, ".glb");
          for (int i = 0; i < mdl.materialCount; i++) {
            mdl.materials[i].shader = shadowShader;
          }
          m_loadedAircraftModels[stats.modelFile] = mdl;
        } else {
          Model fallback = LoadModelFromMesh(GenMeshCube(1.5f, 0.5f, 4.0f));
          fallback.materials[0].shader = shadowShader;
          fallback.materials[0].maps[MATERIAL_MAP_ALBEDO].color = GRAY;
          m_loadedAircraftModels[stats.modelFile] = fallback;
        }
      }
    }
  }

  std::string startingPlane = FlightSession::GetInstance().selectedPlaneId;
  if (!ids.empty()) {
    m_currentPlaneIndex = 0;
    SwitchAircraft(startingPlane.empty() ? ids[0] : startingPlane);
  }

  auto enemyJetData = assetLoader.getAssetData("jet_mk2.glb");
  if (!enemyJetData.empty()) {
    m_enemyJetModel = assetLoader.LoadModelFromAssetData(enemyJetData, ".glb");
    for (int i = 0; i < m_enemyJetModel.materialCount; i++) {
      m_enemyJetModel.materials[i].shader = shadowShader;
      m_enemyJetModel.materials[i].maps[MATERIAL_MAP_ALBEDO].color =
          Color{255, 65, 65, 255};
    }
    m_enemyJetModelLoaded = true;
  }

  m_cargoModel = LoadModelFromMesh(GenMeshCube(4.0f, 4.0f, 4.0f));
  m_cargoModel.materials[0].shader = shadowShader;
  m_cargoModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = ORANGE;
  m_cargoModelLoaded = true;

  // Particle System
  std::vector<char> vsData =
      assetLoader.getAssetData("glsl330/instanced_particle.vs");
  std::vector<char> fsData =
      assetLoader.getAssetData("glsl330/instanced_particle.fs");

  if (vsData.empty() || fsData.empty()) {
    vsData = assetLoader.getAssetData("instanced_particle.vs");
    fsData = assetLoader.getAssetData("instanced_particle.fs");
  }

  if (!vsData.empty() && !fsData.empty()) {
    SaveFileData("temp_vfx_vs.glsl", vsData.data(), (int)vsData.size());
    SaveFileData("temp_vfx_fs.glsl", fsData.data(), (int)fsData.size());
    m_particleShader = LoadShader("temp_vfx_vs.glsl", "temp_vfx_fs.glsl");
    remove("temp_vfx_vs.glsl");
    remove("temp_vfx_fs.glsl");

    Image img = GenImageColor(1, 1, WHITE);
    m_particleTexture = LoadTextureFromImage(img);
    UnloadImage(img);

    if (m_particleShader.id != 0) {
      m_particleSystem.Initialize(m_particleTexture, m_particleShader);
      m_particlesInitialized = true;

      m_explosionEmitter =
          m_particleSystem.AddEmitter(ParticlePresets::Sparks());
      m_shockwaveEmitter =
          m_particleSystem.AddEmitter(ParticlePresets::Shockwave());
      m_runwaySmokeEmitter =
          m_particleSystem.AddEmitter(ParticlePresets::Smoke());

      EmitterConfig missileTrailCfg;
      missileTrailCfg.maxParticles = 1200;
      missileTrailCfg.emissionMode = EmissionMode::BURST;
      missileTrailCfg.lifetimeRange = {0.25f, 0.60f};
      missileTrailCfg.speedRange = {1.0f, 8.0f};
      missileTrailCfg.startSize = 1.6f;
      missileTrailCfg.endSize = 3.2f;
      missileTrailCfg.startColor = Color{255, 140, 40, 220};
      missileTrailCfg.endColor = Color{80, 80, 80, 0};
      missileTrailCfg.drag = 0.8f;
      missileTrailCfg.randomRotation = true;
      m_missileSmokeEmitter = m_particleSystem.AddEmitter(missileTrailCfg);

      EmitterConfig sparkCfg;
      sparkCfg.maxParticles = 600;
      sparkCfg.emissionMode = EmissionMode::BURST;
      sparkCfg.lifetimeRange = {0.10f, 0.35f};
      sparkCfg.speedRange = {10.0f, 40.0f};
      sparkCfg.startSize = 0.8f;
      sparkCfg.endSize = 0.05f;
      sparkCfg.startColor = Color{255, 255, 200, 255};
      sparkCfg.endColor = Color{255, 120, 20, 0};
      sparkCfg.drag = 2.0f;
      m_bulletSparkEmitter = m_particleSystem.AddEmitter(sparkCfg);
    }
  }

  // Mission System
  assetLoader.loadPackedFile("DATA/MISSIONS/missions.bin");
  auto missionBin = assetLoader.getAssetData("missions.xml");

  if (!missionBin.empty()) {
    MissionDatabase::GetInstance().LoadFromMemory(missionBin.data(),
                                                  missionBin.size());
  } else {
    MissionDatabase::GetInstance().LoadFromFile("DATA/MISSIONS/missions.xml");
  }

  m_missionManager.Initialize(m_engine);

  m_missionManager.SetSetupCallback(
      [this](const std::string &planeId, Vector3 spawnPos, float headingDeg) {
        if (!planeId.empty()) {
          SwitchAircraft(planeId);
        }
        m_aircraft.Reset(spawnPos);
        m_aircraft.orientation =
            QuaternionFromEuler(0.0f, -headingDeg * DEG2RAD, 0.0f);
        m_aircraft.isOnGround = true;
        m_aircraft.health = 100.0f;
        m_throttle = 0.0f;

        // Reset engine emergency state on mission start
        m_engineFailed = false;
        m_engineSmokeTimer = 0.0f;

        m_droppedPayloads.clear();
        m_bullets.clear();
        m_missiles.clear();
        m_enemyJets.clear();

        const MissionData *curMission = m_missionManager.GetCurrentMission();
        if (curMission) {
          for (const auto &eData : curMission->enemies) {
            EnemyAce ace;
            ace.id = eData.id;
            ace.name = eData.name;
            ace.position = eData.position;
            ace.orientation = QuaternionFromEuler(0.0f, 180.0f * DEG2RAD, 0.0f);
            ace.velocity = {0.0f, 0.0f, -eData.speed};
            ace.health = eData.health;
            ace.maxHealth = eData.maxHealth;
            ace.hitRadius = eData.radius;
            ace.isAlive = true;
            ace.gunCooldown = 0.0f;
            ace.missileCooldown = 1.0f;
            ace.evadeTimer = 0.0f;
            m_enemyJets.push_back(ace);
          }

          if (curMission->id == "canyon_strike_02" ||
              curMission->requiredPlaneId == "jet_mk2") {
            Vector3 rFwd = m_aircraft.GetForward();
            Vector3 rRgt = m_aircraft.GetRight();

            Vector3 leftCrater =
                Vector3Add(spawnPos, Vector3Add(Vector3Scale(rFwd, 150.0f),
                                                Vector3Scale(rRgt, -35.0f)));
            Vector3 rightCrater =
                Vector3Add(spawnPos, Vector3Add(Vector3Scale(rFwd, 280.0f),
                                                Vector3Scale(rRgt, 40.0f)));

            TriggerExplosionFX(leftCrater, 1.6f);
            TriggerExplosionFX(rightCrater, 2.0f);
          }
        }
      });

  m_missionManager.SetEventCallback(
      [this](const std::string &eventName, const Vector3 &stagePos) {
        if (eventName == "DROP_CARGO") {
          DroppedPayload p;
          p.position =
              Vector3Subtract(m_aircraft.position, Vector3{0.0f, 2.5f, 0.0f});
          p.velocity = m_aircraft.velocity;
          p.active = true;
          p.inWater = false;
          m_droppedPayloads.push_back(p);
        } else if (eventName == "ENGINE_FAIL") {
          m_engineFailed = true;
          m_throttle = 0.0f;
          TriggerExplosionFX(m_aircraft.position, 1.4f);
        }
      });

  // Start chosen mission from MissionSelectScene, or fallback to first
  auto missionIds = MissionDatabase::GetInstance().GetAllMissionIds();
  if (!missionIds.empty()) {
    std::string targetMission = FlightSession::GetInstance().activeMissionId;
    if (targetMission.empty()) {
      targetMission = missionIds[0];
    }
    m_missionManager.StartMission(targetMission);
  }
}

void BudgetFlightSimulatorScene::UpdateDogfight(float deltaTime) {
  for (auto &enemy : m_enemyJets) {
    if (!enemy.isAlive)
      continue;

    if (enemy.gunCooldown > 0.0f)
      enemy.gunCooldown -= deltaTime;
    if (enemy.missileCooldown > 0.0f)
      enemy.missileCooldown -= deltaTime;

    // Current orientation vectors
    Vector3 enemyFwd = Vector3RotateByQuaternion(Vector3{0.0f, 0.0f, -1.0f},
                                                 enemy.orientation);
    Vector3 enemyUp =
        Vector3RotateByQuaternion(Vector3{0.0f, 1.0f, 0.0f}, enemy.orientation);

    // Geometry relative to player
    Vector3 toPlayer = Vector3Subtract(m_aircraft.position, enemy.position);
    float distToPlayer = Vector3Length(toPlayer);
    Vector3 dirToPlayer = (distToPlayer > 0.001f)
                              ? Vector3Scale(toPlayer, 1.0f / distToPlayer)
                              : enemyFwd;

    Vector3 playerFwd = Vector3RotateByQuaternion(Vector3{0.0f, 0.0f, -1.0f},
                                                  m_aircraft.orientation);
    Vector3 playerToEnemy = Vector3Scale(toPlayer, -1.0f);
    Vector3 dirPlayerToEnemy =
        (distToPlayer > 0.001f)
            ? Vector3Scale(playerToEnemy, 1.0f / distToPlayer)
            : Vector3{0, 0, 1};

    float enemyNoseAlignment = Vector3DotProduct(enemyFwd, dirToPlayer);
    float playerNoseAlignment = Vector3DotProduct(playerFwd, dirPlayerToEnemy);

    if (enemyNoseAlignment < 0.1f && playerNoseAlignment > 0.5f) {
      enemy.evadeTimer = 2.8f; // Commit to defensive flying
    }
    if (enemy.evadeTimer > 0.0f) {
      enemy.evadeTimer -= deltaTime;
    }

    Vector3 desiredDir;

    if (enemy.evadeTimer > 0.0f) {
      // DEFENSIVE BREAK: Break hard into a diving turn or climbing spiral
      Vector3 breakDir =
          Vector3CrossProduct(enemyFwd, Vector3{0.0f, 1.0f, 0.0f});
      if (Vector3Length(breakDir) < 0.01f)
        breakDir = Vector3{1.0f, 0.0f, 0.0f};

      desiredDir =
          Vector3Normalize(Vector3Add(enemyFwd, Vector3Scale(breakDir, 1.1f)));
      desiredDir =
          Vector3Normalize(Vector3Add(desiredDir, Vector3{0.0f, 0.15f, 0.0f}));
    } else if (distToPlayer < 240.0f) {
      // OVERSHOOT / REPOSITION: Break upward and forward to reset attack run
      desiredDir =
          Vector3Normalize(Vector3Add(enemyFwd, Vector3{0.0f, 0.5f, 0.0f}));
    } else {
      Vector3 imperfectTarget = Vector3Add(
          m_aircraft.position, Vector3Scale(m_aircraft.velocity, 0.25f));
      Vector3 toTarget = Vector3Subtract(imperfectTarget, enemy.position);
      float targetDist = Vector3Length(toTarget);
      desiredDir = (targetDist > 1.0f)
                       ? Vector3Scale(toTarget, 1.0f / targetDist)
                       : enemyFwd;
    }

    float forwardSpeed = Vector3Length(enemy.velocity);
    float lookAheadDist = fmaxf(forwardSpeed * 2.0f, 220.0f);
    Vector3 probePos =
        Vector3Add(enemy.position, Vector3Scale(enemyFwd, lookAheadDist));

    float currentGroundH =
        m_terrain.GetHeightAt(enemy.position.x, enemy.position.z);
    float futureGroundH = m_terrain.GetHeightAt(probePos.x, probePos.z);
    float minSafeAlt = fmaxf(futureGroundH, 1.0f) + 90.0f;

    if (probePos.y < minSafeAlt ||
        enemy.position.y < (currentGroundH + 70.0f)) {
      float pullUpStrength =
          Clamp((minSafeAlt - probePos.y) / 45.0f, 1.0f, 3.5f);
      desiredDir = Vector3Normalize(
          Vector3Add(desiredDir, Vector3{0.0f, pullUpStrength, 0.0f}));
    }

    Vector3 right = Vector3Normalize(
        Vector3CrossProduct(desiredDir, Vector3{0.0f, 1.0f, 0.0f}));
    if (Vector3Length(right) < 0.01f) {
      right = Vector3RotateByQuaternion(Vector3{1.0f, 0.0f, 0.0f},
                                        enemy.orientation);
    }
    Vector3 bankUp = Vector3Normalize(Vector3CrossProduct(right, desiredDir));

    Matrix targetMat = {right.x, bankUp.x, -desiredDir.x, 0.0f,
                        right.y, bankUp.y, -desiredDir.y, 0.0f,
                        right.z, bankUp.z, -desiredDir.z, 0.0f,
                        0.0f,    0.0f,     0.0f,          1.0f};
    Quaternion targetQuat = QuaternionFromMatrix(targetMat);

    float angleToTarget =
        acosf(Clamp(Vector3DotProduct(enemyFwd, desiredDir), -1.0f, 1.0f));
    float maxTurnRate = 0.46f;
    float maxStep = maxTurnRate * deltaTime;
    float slerpFactor =
        (angleToTarget > 0.001f) ? fminf(maxStep / angleToTarget, 1.0f) : 1.0f;

    enemy.orientation = QuaternionNormalize(
        QuaternionSlerp(enemy.orientation, targetQuat, slerpFactor));
    enemyFwd = Vector3RotateByQuaternion(Vector3{0.0f, 0.0f, -1.0f},
                                         enemy.orientation);

    float throttle = (enemy.evadeTimer > 0.0f) ? 165.0f : 140.0f;
    enemy.velocity = Vector3Scale(enemyFwd, throttle);
    enemy.position =
        Vector3Add(enemy.position, Vector3Scale(enemy.velocity, deltaTime));

    float hardDeck = fmaxf(currentGroundH, 1.0f) + 30.0f;
    if (enemy.position.y < hardDeck) {
      enemy.position.y = hardDeck;
      if (enemy.velocity.y < 0.0f)
        enemy.velocity.y = 0.0f;
    }

    float gunAlignment = Vector3DotProduct(enemyFwd, dirToPlayer);

    // Guns: Only fire when not evading, inside 450m, tight alignment
    if (enemy.evadeTimer <= 0.0f && gunAlignment > 0.965f &&
        distToPlayer < 450.0f && enemy.gunCooldown <= 0.0f) {
      enemy.gunCooldown = 0.12f;
      BulletTracer b;
      b.position = Vector3Add(enemy.position, Vector3Scale(enemyFwd, 6.0f));
      b.velocity = Vector3Scale(enemyFwd, 1100.0f);
      b.life = 1.4f;
      b.isEnemy = true;
      b.active = true;
      m_bullets.push_back(b);
    }

    // Missiles: 8-second reload, won't fire during defensive evasion
    if (enemy.evadeTimer <= 0.0f && gunAlignment > 0.88f &&
        distToPlayer >= 800.0f && distToPlayer <= 1300.0f &&
        enemy.missileCooldown <= 0.0f) {
      enemy.missileCooldown = 8.0f;

      HomingMissile m;
      m.position = Vector3Subtract(enemy.position, Vector3{0.0f, 1.2f, 0.0f});
      m.velocity = Vector3Scale(enemyFwd, 180.0f);
      m.targetPos = m_aircraft.position;
      m.isTrackingPlayer = true;
      m.speed = 340.0f;
      m.turnRate = 3.6f;
      m.life = 6.0f;
      m.blastRadius = 20.0f;
      m.damage = 40.0f;
      m.active = true;
      m_missiles.push_back(m);
    }
  }
}

void BudgetFlightSimulatorScene::UpdateProjectiles(float deltaTime) {
  // 1. Update Minigun Tracer Rounds
  for (auto &b : m_bullets) {
    if (!b.active)
      continue;

    b.life -= deltaTime;
    if (b.life <= 0.0f) {
      b.active = false;
      continue;
    }

    b.position = Vector3Add(b.position, Vector3Scale(b.velocity, deltaTime));

    if (!b.isEnemy) {
      for (auto &enemy : m_enemyJets) {
        if (!enemy.isAlive)
          continue;
        if (Vector3Distance(b.position, enemy.position) <= enemy.hitRadius) {
          b.active = false;
          enemy.health -= m_aircraft.stats.bulletDamage;
          if (m_bulletSparkEmitter)
            m_bulletSparkEmitter->Emit(15, b.position);

          if (enemy.health <= 0.0f) {
            enemy.isAlive = false;
            TriggerExplosionFX(enemy.position, 2.0f);
          }
          break;
        }
      }
    } else {
      if (Vector3Distance(b.position, m_aircraft.position) <= 8.0f) {
        b.active = false;
        m_aircraft.health -= 6.0f;
        if (m_bulletSparkEmitter)
          m_bulletSparkEmitter->Emit(18, b.position);
        if (m_aircraft.health <= 0.0f) {
          TriggerExplosionFX(m_aircraft.position, 2.5f);
        }
      }
    }
  }

  m_bullets.erase(
      std::remove_if(m_bullets.begin(), m_bullets.end(),
                     [](const BulletTracer &b) { return !b.active; }),
      m_bullets.end());

  // 2. Update Homing Missiles with Realistic Seeker Limits & Dodge Windows
  for (auto &m : m_missiles) {
    if (!m.active)
      continue;

    m.life -= deltaTime;
    if (m.life <= 0.0f) {
      m.active = false;
      TriggerExplosionFX(m.position, 1.0f);
      continue;
    }

    if (m_missileSmokeEmitter)
      m_missileSmokeEmitter->Emit(2, m.position);

    Vector3 currentDir = Vector3Normalize(m.velocity);
    float distToTarget = 0.0f;
    float effectiveTurnRate = 2.4f;

    if (m.isTrackingPlayer) {
      Vector3 toPlayer = Vector3Subtract(m_aircraft.position, m.position);
      distToTarget = Vector3Length(toPlayer);
      Vector3 dirToPlayer = (distToTarget > 0.001f)
                                ? Vector3Scale(toPlayer, 1.0f / distToTarget)
                                : currentDir;

      // 1. Seeker FOV Limit
      float seekerAlignment = Vector3DotProduct(currentDir, dirToPlayer);
      if (seekerAlignment < 0.67f) {
        m.isTrackingPlayer = false;
      }

      // 2. Terminal Guidance Freeze Window
      if (distToTarget < 75.0f) {
        effectiveTurnRate = 0.4f;
      }

      // 3. Tangential Evasion Bonus
      Vector3 playerLateralVel = Vector3Subtract(
          m_aircraft.velocity,
          Vector3Scale(dirToPlayer,
                       Vector3DotProduct(m_aircraft.velocity, dirToPlayer)));
      float crossVelocity = Vector3Length(playerLateralVel);

      float evasionBonus = (crossVelocity * 0.45f) +
                           (fabsf(m_aircraft.angularVelocity.x) * 12.0f);

      float timeToHit =
          Clamp(distToTarget / fmaxf(m.speed, 100.0f), 0.0f, 2.0f);
      Vector3 imperfectLead =
          Vector3Scale(m_aircraft.velocity, timeToHit * 0.6f);

      float t = (float)GetTime() * 4.0f + m.life;
      Vector3 trackingWander = {sinf(t * 1.5f) * (evasionBonus + 8.0f),
                                cosf(t * 1.2f) * (evasionBonus * 0.5f + 4.0f),
                                sinf(t * 2.0f) * (evasionBonus + 8.0f)};

      m.targetPos = Vector3Add(Vector3Add(m_aircraft.position, imperfectLead),
                               trackingWander);

    } else {
      float closestDist = 99999.0f;
      for (const auto &enemy : m_enemyJets) {
        if (!enemy.isAlive)
          continue;
        float d = Vector3Distance(m.position, enemy.position);
        if (d < closestDist) {
          closestDist = d;
          m.targetPos = enemy.position;
        }
      }
      distToTarget = closestDist;
    }

    // Steering Execution
    Vector3 toTarget = Vector3Subtract(m.targetPos, m.position);
    float lenToTarget = Vector3Length(toTarget);

    if (lenToTarget > 1.0f) {
      Vector3 desiredDir = Vector3Scale(toTarget, 1.0f / lenToTarget);

      float angleDiff =
          acosf(Clamp(Vector3DotProduct(currentDir, desiredDir), -1.0f, 1.0f));
      float maxAngleStep = effectiveTurnRate * deltaTime;

      Vector3 steerDir;
      if (angleDiff > 0.001f) {
        float t = fminf(maxAngleStep / angleDiff, 1.0f);
        steerDir = Vector3Normalize(Vector3Lerp(currentDir, desiredDir, t));
      } else {
        steerDir = desiredDir;
      }

      m.velocity = Vector3Scale(steerDir, m.speed);
    }

    m.position = Vector3Add(m.position, Vector3Scale(m.velocity, deltaTime));

    // Proximity Detonation
    float tunedBlastRadius = 10.0f;

    if (m.isTrackingPlayer) {
      if (Vector3Distance(m.position, m_aircraft.position) <=
          tunedBlastRadius) {
        m.active = false;
        m_aircraft.health -= m.damage;
        TriggerExplosionFX(m.position, 2.4f);
      }
    } else {
      for (auto &enemy : m_enemyJets) {
        if (!enemy.isAlive)
          continue;
        if (Vector3Distance(m.position, enemy.position) <= tunedBlastRadius) {
          m.active = false;
          enemy.health -= m.damage;
          TriggerExplosionFX(m.position, 2.0f);

          if (enemy.health <= 0.0f) {
            enemy.isAlive = false;
            TriggerExplosionFX(enemy.position, 2.6f);
          }
          break;
        }
      }
    }
  }

  m_missiles.erase(
      std::remove_if(m_missiles.begin(), m_missiles.end(),
                     [](const HomingMissile &m) { return !m.active; }),
      m_missiles.end());
}

void BudgetFlightSimulatorScene::Update(float deltaTime) {
  if (IsKeyPressed(KEY_F)) {
    m_isFrozen = !m_isFrozen;
  }

  if (IsKeyPressed(KEY_ESCAPE)) {
    m_engine->GetSceneManager().LoadScene("MissionSelectScene");
    return;
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
      IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    DisableCursor();
  }

  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) ||
      IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
        !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      EnableCursor();
    }
  }

  const auto &planeIds = AircraftDatabase::GetInstance().GetAllPlaneIds();

  if (IsKeyPressed(KEY_ONE))
    SwitchAircraft("trainer");
  if (IsKeyPressed(KEY_TWO))
    SwitchAircraft("jet_mk2");
  if (IsKeyPressed(KEY_THREE))
    SwitchAircraft("cargo");

  if (!planeIds.empty()) {
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
      m_currentPlaneIndex = (m_currentPlaneIndex + 1) % planeIds.size();
      SwitchAircraft(planeIds[m_currentPlaneIndex]);
    }
    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
      m_currentPlaneIndex =
          (m_currentPlaneIndex + planeIds.size() - 1) % planeIds.size();
      SwitchAircraft(planeIds[m_currentPlaneIndex]);
    }
  }

  if (IsKeyPressed(KEY_F5)) {
    auto xmlData = m_engine->GetAssetLoader().getAssetData("aircraft.xml");
    if (!xmlData.empty()) {
      AircraftDatabase::GetInstance().LoadFromMemory(xmlData.data(),
                                                     xmlData.size());
    } else {
      AircraftDatabase::GetInstance().LoadFromFile("assets/data/aircraft.xml");
    }
    if (!planeIds.empty()) {
      SwitchAircraft(planeIds[m_currentPlaneIndex]);
    }
  }

  // Weather Hotkeys
  auto &weatherSys = m_engine->GetWeatherSystem();
  if (IsKeyPressed(KEY_L)) {
    weatherSys.CycleNextWeather(4.0f);
    weatherSys.ResetStateTimer();
  }
  if (IsKeyPressed(KEY_K)) {
    int prev =
        ((int)weatherSys.GetCurrentPreset() + (int)WeatherPreset::COUNT - 1) %
        (int)WeatherPreset::COUNT;
    weatherSys.SetWeather((WeatherPreset)prev, 3.5f);
    weatherSys.ResetStateTimer();
  }

  // Mission Hotkeys
  const auto &missionIds = MissionDatabase::GetInstance().GetAllMissionIds();
  static int currentMissionIndex = 0;
  if (!missionIds.empty()) {
    if (IsKeyPressed(KEY_N)) {
      currentMissionIndex = (currentMissionIndex + 1) % missionIds.size();
      m_missionManager.StartMission(missionIds[currentMissionIndex]);
    }
    if (IsKeyPressed(KEY_M)) {
      currentMissionIndex = (currentMissionIndex + (int)missionIds.size() - 1) %
                            missionIds.size();
      m_missionManager.StartMission(missionIds[currentMissionIndex]);
    }
  }

  weatherSys.Update(deltaTime, m_flightCamera.GetRaylibCamera().position,
                    m_aircraft.velocity);

  auto &dayNight = m_engine->GetDayNightCycle();
  dayNight.Update(deltaTime);

  if (IsKeyDown(KEY_P))
    dayNight.SetTime(dayNight.GetTime() + 1.0f * deltaTime);
  if (IsKeyDown(KEY_O))
    dayNight.SetTime(dayNight.GetTime() - 1.0f * deltaTime);

  // Sync Shaders
  Shader shadowShader = m_engine->GetRenderer().GetShadowShader();
  Shader skyShader = m_engine->GetRenderer().GetSkyShader();
  dayNight.ApplyToShaders(shadowShader, skyShader);

  const auto &weatherProf = weatherSys.GetActiveProfile();
  Vector4 dimmedSun = Vector4Scale(dayNight.GetCurrentLighting().lightColor,
                                   weatherProf.sunDimFactor);
  int lightColLoc = GetShaderLocation(shadowShader, "lightColor");
  if (lightColLoc != -1) {
    SetShaderValue(shadowShader, lightColLoc, &dimmedSun, SHADER_UNIFORM_VEC4);
  }

  if (skyShader.id > 0) {
    int covLoc = GetShaderLocation(skyShader, "u_cloudCover");
    if (covLoc != -1)
      SetShaderValue(skyShader, covLoc, &weatherProf.cloudCover,
                     SHADER_UNIFORM_FLOAT);
    int falLoc = GetShaderLocation(skyShader, "u_cloudFalloff");
    if (falLoc != -1)
      SetShaderValue(skyShader, falLoc, &weatherProf.cloudFalloff,
                     SHADER_UNIFORM_FLOAT);
    int fogLoc = GetShaderLocation(skyShader, "u_fogDensity");
    if (fogLoc != -1)
      SetShaderValue(skyShader, fogLoc, &weatherProf.fogDensity,
                     SHADER_UNIFORM_FLOAT);
    int horLoc = GetShaderLocation(skyShader, "u_horizonColor");
    if (horLoc != -1) {
      Vector3 blendedHor =
          Vector3Multiply(dayNight.GetCurrentLighting().equatorColor,
                          weatherProf.horizonHazeTint);
      SetShaderValue(skyShader, horLoc, &blendedHor, SHADER_UNIFORM_VEC3);
    }
  }

  // Atmospheric Flight Turbulence
  if (!m_isFrozen && weatherProf.turbulence > 0.05f) {
    float tTime = (float)GetTime() * 3.5f;
    float rollWiggle = sinf(tTime * 1.4f) * cosf(tTime * 0.9f) *
                       (weatherProf.turbulence * 0.25f) * deltaTime;
    float pitchWiggle =
        cosf(tTime * 1.1f) * (weatherProf.turbulence * 0.15f) * deltaTime;
    Quaternion perturb = QuaternionFromEuler(pitchWiggle, 0.0f, rollWiggle);
    m_aircraft.orientation = QuaternionNormalize(
        QuaternionMultiply(m_aircraft.orientation, perturb));
  }

  if (!m_isFrozen) {
    FlightInput input;
    if (IsKeyDown(KEY_W))
      input.pitch = -1.0f;
    if (IsKeyDown(KEY_S))
      input.pitch = 1.0f;
    if (IsKeyDown(KEY_A))
      input.roll = -1.0f;
    if (IsKeyDown(KEY_D))
      input.roll = 1.0f;
    if (IsKeyDown(KEY_Q))
      input.yaw = 1.0f;
    if (IsKeyDown(KEY_E))
      input.yaw = -1.0f;

    input.brake = IsKeyDown(KEY_SPACE) && !m_aircraft.stats.hasWeapons;

    // Throttle lockout on engine failure
    if (!m_engineFailed) {
      if (IsKeyDown(KEY_LEFT_SHIFT))
        m_throttle += 0.35f * deltaTime;
      if (IsKeyDown(KEY_X) ||
          (!m_aircraft.stats.hasWeapons && IsKeyDown(KEY_LEFT_CONTROL))) {
        m_throttle -= 0.35f * deltaTime;
      }
    } else {
      m_throttle = 0.0f;
    }

    m_throttle = Clamp(m_throttle, 0.0f, 1.0f);
    input.throttle = m_throttle;

    // Persistent engine smoke emission for the Cargo Plane
    if (m_engineFailed && m_aircraft.health > 0.0f) {
      Vector3 planePos = m_aircraft.position;
      Vector3 fwd = m_aircraft.GetForward();
      Vector3 right = m_aircraft.GetRight();
      Vector3 up = m_aircraft.GetUp();

      const float innerNacelleSpan = 14.5f;
      const float foreAftOffset = 50.0f;
      const float verticalDrop = 1.0f;

      Vector3 innerLeftEngine = Vector3Add(
          planePos, Vector3Add(Vector3Scale(right, -innerNacelleSpan),
                               Vector3Add(Vector3Scale(fwd, foreAftOffset),
                                          Vector3Scale(up, -verticalDrop))));

      Vector3 innerRightEngine = Vector3Add(
          planePos, Vector3Add(Vector3Scale(right, innerNacelleSpan),
                               Vector3Add(Vector3Scale(fwd, foreAftOffset),
                                          Vector3Scale(up, -verticalDrop))));

      if (m_runwaySmokeEmitter) {
        m_runwaySmokeEmitter->Emit(3, innerLeftEngine);
        m_runwaySmokeEmitter->Emit(3, innerRightEngine);
      }

      if (m_bulletSparkEmitter && ((rand() % 8) == 0)) {
        m_bulletSparkEmitter->Emit(5, innerLeftEngine);
      }
    }

    m_aircraft.Update(input, deltaTime);
    m_aircraft.UpdateWeaponTimers(deltaTime);

    // Propeller RPM
    float propRpmDeg = (720.0f + (m_throttle * 3600.0f)) * deltaTime;
    m_propellerAngleDeg = fmodf(m_propellerAngleDeg + propRpmDeg, 360.0f);

    if (m_aircraft.stats.hasWeapons) {
      if (IsKeyDown(KEY_SPACE)) {
        m_aircraft.FireMinigun(m_bullets);
      }

      if (IsKeyPressed(KEY_LEFT_CONTROL)) {
        Vector3 lockTarget =
            Vector3Add(m_aircraft.position,
                       Vector3Scale(m_aircraft.GetForward(), 1000.0f));
        for (const auto &enemy : m_enemyJets) {
          if (enemy.isAlive) {
            lockTarget = enemy.position;
            break;
          }
        }
        m_aircraft.FireMissile(m_missiles, lockTarget);
      }
    }
  }

  // Terrain Clamping & Crash Checks
  float groundHeight =
      m_terrain.GetHeightAt(m_aircraft.position.x, m_aircraft.position.z);
  float minAltitude = groundHeight + 1.5f;

  if (m_aircraft.position.y <= minAltitude) {
    if (!m_aircraft.isOnGround &&
        (m_aircraft.velocity.y < -18.0f ||
         (m_aircraft.GetPitchAngle() < -25.0f &&
          Vector3Length(m_aircraft.velocity) > 60.0f))) {
      TriggerExplosionFX(m_aircraft.position, 2.5f);
      m_aircraft.health = 0.0f;
    }
    m_aircraft.position.y = minAltitude;
    m_aircraft.isOnGround = true;
    if (m_aircraft.velocity.y < 0.0f)
      m_aircraft.velocity.y = 0.0f;
  } else {
    m_aircraft.isOnGround = false;
  }

  // 1. Update Camera Physics Base
  m_flightCamera.Update(m_aircraft, deltaTime);
  Camera3D flyCam = m_flightCamera.GetRaylibCamera();

  // 2. Smooth Damped Ground & Sea Anti-Clipping
  float camGroundH =
      m_terrain.GetHeightAt(flyCam.position.x, flyCam.position.z);
  float minAllowedY = fmaxf(camGroundH, 1.0f) + 1.8f;

  if (flyCam.position.y < minAllowedY) {
    flyCam.position.y = Lerp(flyCam.position.y, minAllowedY,
                             Clamp(12.0f * deltaTime, 0.0f, 1.0f));
  }

  // 3. Screen Shake Deceleration
  if (m_screenShakeIntensity > 0.0f) {
    m_screenShakeIntensity -= 4.0f * deltaTime;
    if (m_screenShakeIntensity < 0.0f)
      m_screenShakeIntensity = 0.0f;
  }
  if (m_explosionPostFXTimer > 0.0f) {
    m_explosionPostFXTimer -= deltaTime;
  }

  // 4. Synchronize Smoothed State to Engine Camera
  Camera &engineCam = m_engine->GetCamera();
  if (m_screenShakeIntensity > 0.01f) {
    flyCam.position.x +=
        (((float)rand() / (float)RAND_MAX) - 0.5f) * m_screenShakeIntensity;
    flyCam.position.y +=
        (((float)rand() / (float)RAND_MAX) - 0.5f) * m_screenShakeIntensity;
  }

  engineCam.position = flyCam.position;
  engineCam.target = flyCam.target;
  engineCam.up = flyCam.up;
  engineCam.fovy = flyCam.fovy;
  engineCam.projection = flyCam.projection;

  Vector3 skySun = Vector3Negate(dayNight.GetCurrentLighting().lightDir);
  m_ocean.Update(deltaTime, engineCam, skySun);

  UpdateDogfight(deltaTime);
  UpdateProjectiles(deltaTime);

  for (auto &p : m_droppedPayloads) {
    if (!p.active)
      continue;
    if (!p.inWater) {
      p.velocity.y -= 9.8f * 1.6f * deltaTime;
      p.velocity.x *= 0.992f;
      p.velocity.z *= 0.992f;
      p.position = Vector3Add(p.position, Vector3Scale(p.velocity, deltaTime));

      float groundH = m_terrain.GetHeightAt(p.position.x, p.position.z);
      if (p.position.y <= groundH) {
        p.position.y = groundH + 2.0f;
        p.velocity = Vector3Zero();
        p.inWater = (groundH <= 1.0f);
      }
    }
  }

  if (m_particlesInitialized) {
    m_particleSystem.Update(deltaTime);
  }

  bool enemyEliminated = false;
  for (const auto &enemy : m_enemyJets) {
    if (!enemy.isAlive)
      enemyEliminated = true;
  }

  Shader currentPostShader = m_engine->GetPostProcessing().GetCurrentShader();
  Vector2 rRes = {(float)GetRenderWidth(), (float)GetRenderHeight()};
  int resLoc = GetShaderLocation(currentPostShader, "resolution");
  if (resLoc != -1) {
    SetShaderValue(currentPostShader, resLoc, &rRes, SHADER_UNIFORM_VEC2);
  }

  int expLoc = GetShaderLocation(currentPostShader, "u_explosionFactor");
  if (expLoc != -1) {
    float factor = Clamp(m_explosionPostFXTimer / 0.45f, 0.0f, 1.0f);
    SetShaderValue(currentPostShader, expLoc, &factor, SHADER_UNIFORM_FLOAT);
  }

  bool actionKeyPressed = IsKeyPressed(KEY_G);
  m_missionManager.Update(deltaTime, m_aircraft.position,
                          m_aircraft.GetAirspeedKmh(),
                          m_aircraft.GetAltitudeMeters(), m_aircraft.isOnGround,
                          actionKeyPressed, enemyEliminated);

  // --- PROGRESS TRACKING INTEGRATION ---
  static bool missionSaved = false;
  if (m_missionManager.IsCompleted() && !missionSaved) {
    missionSaved = true;
    PlayerProgress progress;
    progress.Load("player.save");

    // Match the current mission ID to its numerical index
    const auto &allMissions = MissionDatabase::GetInstance().GetAllMissionIds();
    const MissionData *curData = m_missionManager.GetCurrentMission();
    int curIndex = 0;
    if (curData) {
      for (size_t i = 0; i < allMissions.size(); ++i) {
        if (allMissions[i] == curData->id) {
          curIndex = (int)i;
          break;
        }
      }
    }

    // Ensure array is properly sized
    while (progress.levels.size() <= (size_t)curIndex) {
      LevelStat stat;
      stat.unlocked = (progress.levels.empty());
      stat.stars = 0;
      stat.bestTime = 0.0f;
      progress.levels.push_back(stat);
    }

    // Calculate stars based on aircraft health remaining
    int stars = (m_aircraft.health >= 80.0f)
                    ? 3
                    : ((m_aircraft.health >= 40.0f) ? 2 : 1);
    float completionTime = m_missionManager.GetMissionTime();

    progress.CompleteLevel(curIndex, completionTime, stars);
  } else if (!m_missionManager.IsCompleted()) {
    missionSaved = false;
  }
}

void BudgetFlightSimulatorScene::DrawRadarHUD(int screenWidth,
                                              int screenHeight) {
  const MissionData *curMission = m_missionManager.GetCurrentMission();

  bool radarEnabled =
      (curMission && curMission->hasRadar) || m_aircraft.stats.hasWeapons;
  if (!radarEnabled)
    return;

  const float radarRadius = 75.0f;
  const float radarRangeM = 3500.0f;
  const float radarMargin = 28.0f;

  const Vector2 radarCenter = {(float)screenWidth - radarRadius - radarMargin,
                               60.0f + radarRadius};

  // Radar Background
  DrawCircleV(radarCenter, radarRadius + 4.0f, Fade(BLACK, 0.45f));
  DrawCircleV(radarCenter, radarRadius, Color{10, 20, 35, 200});
  DrawCircleLines((int)radarCenter.x, (int)radarCenter.y, radarRadius,
                  Color{80, 200, 255, 220});
  DrawCircleLines((int)radarCenter.x, (int)radarCenter.y, radarRadius * 0.66f,
                  Color{40, 100, 150, 120});
  DrawCircleLines((int)radarCenter.x, (int)radarCenter.y, radarRadius * 0.33f,
                  Color{40, 100, 150, 120});

  DrawLineEx(Vector2{radarCenter.x - radarRadius, radarCenter.y},
             Vector2{radarCenter.x + radarRadius, radarCenter.y}, 1.0f,
             Color{40, 100, 150, 140});
  DrawLineEx(Vector2{radarCenter.x, radarCenter.y - radarRadius},
             Vector2{radarCenter.x, radarCenter.y + radarRadius}, 1.0f,
             Color{40, 100, 150, 140});

  float sweepAngle = fmodf((float)GetTime() * 140.0f, 360.0f) * DEG2RAD;
  Vector2 sweepTip = {radarCenter.x + cosf(sweepAngle) * (radarRadius - 2.0f),
                      radarCenter.y + sinf(sweepAngle) * (radarRadius - 2.0f)};
  DrawLineEx(radarCenter, sweepTip, 1.5f,
             Fade(Color{120, 240, 255, 255}, 0.5f));

  DrawTriangle(Vector2{radarCenter.x, radarCenter.y - 6.0f},
               Vector2{radarCenter.x - 4.0f, radarCenter.y + 4.0f},
               Vector2{radarCenter.x + 4.0f, radarCenter.y + 4.0f},
               Color{255, 215, 0, 255});

  Vector3 playerFwd = m_aircraft.GetForward();
  Vector3 playerRight = m_aircraft.GetRight();

  for (const auto &enemy : m_enemyJets) {
    if (!enemy.isAlive)
      continue;

    Vector3 toEnemy = Vector3Subtract(enemy.position, m_aircraft.position);

    float distForward = Vector3DotProduct(toEnemy, playerFwd);
    float distRight = Vector3DotProduct(toEnemy, playerRight);
    float horizDist = sqrtf(distForward * distForward + distRight * distRight);

    float normDist = horizDist / radarRangeM;
    bool outOfRange = (normDist > 1.0f);
    if (outOfRange)
      normDist = 1.0f;

    float blipX =
        radarCenter.x + (distRight / (horizDist > 0.001f ? horizDist : 1.0f)) *
                            normDist * (radarRadius - 6.0f);
    float blipY = radarCenter.y -
                  (distForward / (horizDist > 0.001f ? horizDist : 1.0f)) *
                      normDist * (radarRadius - 6.0f);

    if (outOfRange) {
      DrawCircle((int)blipX, (int)blipY, 2.5f, Fade(RED, 0.6f));
    } else {
      DrawRectanglePro(Rectangle{blipX, blipY, 6.0f, 6.0f}, Vector2{3.0f, 3.0f},
                       45.0f, RED);

      float altDiff = enemy.position.y - m_aircraft.position.y;
      if (altDiff > 50.0f) {
        DrawTriangle(Vector2{blipX, blipY - 6.0f},
                     Vector2{blipX - 3.0f, blipY - 3.0f},
                     Vector2{blipX + 3.0f, blipY - 3.0f}, RED);
      } else if (altDiff < -50.0f) {
        DrawTriangle(Vector2{blipX - 3.0f, blipY + 3.0f},
                     Vector2{blipX, blipY + 6.0f},
                     Vector2{blipX + 3.0f, blipY + 3.0f}, RED);
      }
    }
  }

  DrawUIString("3.5KM", radarCenter.x - 14.0f,
               radarCenter.y + radarRadius - 16.0f, 9.0f,
               Color{80, 200, 255, 200});
}

void BudgetFlightSimulatorScene::DrawProceduralPropeller(
    Vector3 hubPos, float spinAngleDeg, Matrix planeWorldMatrix) {
  Matrix hubTrans = MatrixTranslate(hubPos.x, hubPos.y, hubPos.z);
  Matrix spinMat = MatrixRotateZ(spinAngleDeg * DEG2RAD);
  Matrix engineLocalToWorld =
      MatrixMultiply(MatrixMultiply(spinMat, hubTrans), planeWorldMatrix);

  Vector3 spinnerBase =
      Vector3Transform(Vector3{0.0f, 0.0f, -0.35f}, engineLocalToWorld);
  Vector3 spinnerTip =
      Vector3Transform(Vector3{0.0f, 0.0f, 0.55f}, engineLocalToWorld);
  DrawCylinderEx(spinnerBase, spinnerTip, 0.45f, 0.05f, 8,
                 Color{220, 45, 45, 255});

  const float bladeLength = 2.1f;
  const float bladeWidth = 0.28f;

  for (int b = 0; b < 3; b++) {
    float bladeRotDeg = (float)b * 120.0f;
    Matrix bladeRot = MatrixRotateZ(bladeRotDeg * DEG2RAD);
    Matrix bladeMatrix = MatrixMultiply(bladeRot, engineLocalToWorld);

    Vector3 b0 = Vector3Transform(Vector3{0.0f, 0.25f, 0.0f}, bladeMatrix);
    Vector3 b1 =
        Vector3Transform(Vector3{0.0f, bladeLength * 0.78f, 0.0f}, bladeMatrix);
    DrawCylinderEx(b0, b1, bladeWidth * 0.5f, bladeWidth * 0.4f, 6,
                   Color{240, 185, 35, 255});

    Vector3 b2 =
        Vector3Transform(Vector3{0.0f, bladeLength, 0.0f}, bladeMatrix);
    DrawCylinderEx(b1, b2, bladeWidth * 0.4f, bladeWidth * 0.15f, 6,
                   Color{220, 45, 45, 255});
  }
}

void BudgetFlightSimulatorScene::DrawAnimatedAircraftModel(
    const Model &model, Vector3 position, Quaternion orientation,
    const std::string &planeId) {
  Matrix planeWorldMatrix =
      MatrixMultiply(QuaternionToMatrix(orientation),
                     MatrixTranslate(position.x, position.y, position.z));

  const int trainerPropIdx = 6;

  for (int i = 0; i < model.meshCount; i++) {
    if (planeId == "sea_duck" && (i == 16 || i == 17)) {
      continue;
    }

    Material mat = model.materials[model.meshMaterial[i]];
    Matrix meshMatrix = planeWorldMatrix;

    if (planeId == "trainer" && i == trainerPropIdx) {
      BoundingBox b = GetMeshBoundingBox(model.meshes[i]);
      Vector3 hub = {(b.min.x + b.max.x) * 0.5f, (b.min.y + b.max.y) * 0.5f,
                     (b.min.z + b.max.z) * 0.5f};

      Matrix toOrigin = MatrixTranslate(-hub.x, -hub.y, -hub.z);
      Matrix spin = MatrixRotateZ(m_propellerAngleDeg * DEG2RAD);
      Matrix fromOrigin = MatrixTranslate(hub.x, hub.y, hub.z);

      meshMatrix = MatrixMultiply(
          MatrixMultiply(MatrixMultiply(toOrigin, spin), fromOrigin),
          planeWorldMatrix);
    }

    DrawMesh(model.meshes[i], mat, meshMatrix);
  }

  if (planeId == "sea_duck") {
    const float engineCenterX = 1.38f;
    const float halfSpanX = 5.00f;
    const float engineY = 4.35f;
    const float engineZ = 3.82f;

    float leftX = engineCenterX - halfSpanX;
    float rightX = engineCenterX + halfSpanX;

    DrawProceduralPropeller(Vector3{leftX, engineY, engineZ},
                            m_propellerAngleDeg, planeWorldMatrix);
    DrawProceduralPropeller(Vector3{rightX, engineY, engineZ},
                            -m_propellerAngleDeg, planeWorldMatrix);
  }
}

void BudgetFlightSimulatorScene::Draw3D() {
  Camera3D flyCam = m_engine->GetCamera();
  Shader shadowShader = m_engine->GetRenderer().GetShadowShader();

  rlEnableDepthTest();
  rlEnableDepthMask();
  rlEnableBackfaceCulling();

  rlMatrixMode(RL_PROJECTION);
  rlPushMatrix();
  rlLoadIdentity();
  double aspect = (double)GetScreenWidth() / (double)GetScreenHeight();
  double top = 2.0 * tan(flyCam.fovy * 0.5 * DEG2RAD);
  double right = top * aspect;
  rlFrustum(-right, right, -top, top, 2.0, 15000.0);
  rlMatrixMode(RL_MODELVIEW);

  // 1. Player Aircraft
  if (m_modelLoaded && m_aircraft.health > 0.0f) {
    DrawAnimatedAircraftModel(m_planeModel, m_aircraft.position,
                              m_aircraft.orientation, m_aircraft.stats.id);
  }

  // 2. Hostile Jets
  if (m_enemyJetModelLoaded) {
    for (const auto &enemy : m_enemyJets) {
      if (enemy.isAlive) {
        m_enemyJetModel.transform = QuaternionToMatrix(enemy.orientation);
        DrawModel(m_enemyJetModel, enemy.position, 1.0f, WHITE);
      }
    }
  }

  // 3. Cargo Boxes
  if (m_cargoModelLoaded) {
    for (const auto &p : m_droppedPayloads) {
      if (p.active) {
        DrawModel(m_cargoModel, p.position, 1.0f, WHITE);
        DrawCubeWires(p.position, 4.2f, 4.2f, 4.2f, RED);
      }
    }
  }

  // 4. World Geography
  m_terrain.Draw(shadowShader);
  m_terrain.DrawProps(flyCam);
  m_ocean.Draw();

  rlDrawRenderBatchActive();

  m_engine->GetWeatherSystem().Draw3D(flyCam.position);
  m_missionManager.Draw3D(flyCam);

  for (const auto &b : m_bullets) {
    if (b.active) {
      Vector3 tail = Vector3Subtract(
          b.position, Vector3Scale(Vector3Normalize(b.velocity), 6.0f));
      DrawLine3D(tail, b.position, b.isEnemy ? RED : YELLOW);
    }
  }

  for (const auto &m : m_missiles) {
    if (m.active) {
      Vector3 fwd = Vector3Normalize(m.velocity);
      DrawCylinderEx(m.position,
                     Vector3Add(m.position, Vector3Scale(fwd, 4.0f)), 0.45f,
                     0.45f, 8, m.isTrackingPlayer ? RED : WHITE);
    }
  }

  rlDrawRenderBatchActive();

  // 5. GPU Additive Particles
  if (m_particlesInitialized) {
    m_particleSystem.Draw(flyCam);
  }

  rlEnableDepthTest();
  rlEnableDepthMask();
  rlEnableBackfaceCulling();

  rlMatrixMode(RL_PROJECTION);
  rlPopMatrix();
  rlMatrixMode(RL_MODELVIEW);
}

void BudgetFlightSimulatorScene::DrawUIString(const char *text, float posX,
                                              float posY, float fontSize,
                                              Color color) {
  if (m_customFontLoaded) {
    DrawTextEx(m_hudFont, text, Vector2{posX + 1.2f, posY + 1.2f}, fontSize,
               1.0f, Fade(BLACK, 0.65f));
    DrawTextEx(m_hudFont, text, Vector2{posX, posY}, fontSize, 1.0f, color);
  } else {
    DrawText(text, (int)posX, (int)posY, (int)fontSize, color);
  }
}

float BudgetFlightSimulatorScene::MeasureUIString(const char *text,
                                                  float fontSize) {
  if (m_customFontLoaded) {
    return MeasureTextEx(m_hudFont, text, fontSize, 1.0f).x;
  }
  return (float)MeasureText(text, (int)fontSize);
}

int BudgetFlightSimulatorScene::DrawTextWordWrap(const char *text, int posX,
                                                 int posY, int maxWidth,
                                                 int fontSize, int lineSpacing,
                                                 Color color) {
  if (!text || text[0] == '\0')
    return posY;

  std::string fullText(text);
  std::string currentLine = "";
  std::string currentWord = "";
  int currentY = posY;

  for (size_t i = 0; i <= fullText.length(); ++i) {
    char c = (i < fullText.length()) ? fullText[i] : ' ';

    if (c == ' ' || c == '\n' || i == fullText.length()) {
      if (!currentWord.empty()) {
        std::string testLine = currentLine.empty()
                                   ? currentWord
                                   : (currentLine + " " + currentWord);
        float lineW = MeasureUIString(testLine.c_str(), (float)fontSize);
        if (lineW > (float)maxWidth && !currentLine.empty()) {
          DrawUIString(currentLine.c_str(), (float)posX, (float)currentY,
                       (float)fontSize, color);
          currentY += fontSize + lineSpacing;
          currentLine = currentWord;
        } else {
          currentLine = testLine;
        }
        currentWord.clear();
      }
      if (c == '\n') {
        DrawUIString(currentLine.c_str(), (float)posX, (float)currentY,
                     (float)fontSize, color);
        currentY += fontSize + lineSpacing;
        currentLine.clear();
      }
    } else {
      currentWord += c;
    }
  }

  if (!currentLine.empty()) {
    DrawUIString(currentLine.c_str(), (float)posX, (float)currentY,
                 (float)fontSize, color);
    currentY += fontSize + lineSpacing;
  }

  return currentY;
}

static void DrawGlassPanel(Rectangle rect, float roundness = 0.25f,
                           Color tint = Color{18, 28, 48, 160}) {
  DrawRectangleRounded(
      Rectangle{rect.x + 3.0f, rect.y + 4.0f, rect.width, rect.height},
      roundness, 8, Fade(BLACK, 0.35f));
  DrawRectangleRounded(rect, roundness, 8, tint);
  DrawRectangleRoundedLines(rect, roundness, 8,
                            Fade(Color{130, 220, 255, 230}, 0.85f));
  Rectangle glossRect = {rect.x + 5.0f, rect.y + 3.0f, rect.width - 10.0f,
                         rect.height * 0.30f};
  DrawRectangleRounded(glossRect, 0.5f, 6, Fade(WHITE, 0.12f));
}

void BudgetFlightSimulatorScene::DrawCompassTape(float headingDeg,
                                                 int screenWidth) {
  const float tapeWidth = 380.0f;
  const float tapeHeight = 44.0f;
  const float centerX = screenWidth / 2.0f;
  const float startY = 16.0f;

  Rectangle tapeRect = {centerX - tapeWidth / 2.0f, startY, tapeWidth,
                        tapeHeight};
  DrawGlassPanel(tapeRect, 0.45f, Color{14, 26, 42, 175});

  DrawTriangle(Vector2{centerX, startY + tapeHeight - 3.0f},
               Vector2{centerX - 8.0f, startY + tapeHeight - 14.0f},
               Vector2{centerX + 8.0f, startY + tapeHeight - 14.0f},
               Color{255, 215, 0, 255});

  const float pxPerDeg = 3.6f;
  const int visibleDegrees = (int)((tapeWidth / 2.0f) / pxPerDeg);

  BeginScissorMode((int)tapeRect.x + 6, (int)tapeRect.y + 2,
                   (int)tapeRect.width - 12, (int)tapeRect.height - 4);

  int curHeadingInt = (int)headingDeg;
  for (int offset = -visibleDegrees - 5; offset <= visibleDegrees + 5;
       offset++) {
    int degree = (curHeadingInt + offset) % 360;
    if (degree < 0)
      degree += 360;

    float screenX =
        centerX + (offset * pxPerDeg) - fmodf(headingDeg, 1.0f) * pxPerDeg;

    if (degree % 5 == 0) {
      bool isMajor = (degree % 15 == 0);
      float tickHeight = isMajor ? 14.0f : 7.0f;
      DrawLineEx(Vector2{screenX, startY + 4.0f},
                 Vector2{screenX, startY + 4.0f + tickHeight}, 2.0f,
                 Color{100, 240, 255, 230});

      if (isMajor) {
        const char *label = nullptr;
        if (degree == 0)
          label = "N";
        else if (degree == 90)
          label = "E";
        else if (degree == 180)
          label = "S";
        else if (degree == 270)
          label = "W";
        else
          label = TextFormat("%03d", degree);

        float textW = MeasureUIString(label, 11.0f);
        DrawUIString(label, screenX - textW / 2.0f, startY + 20.0f, 11.0f,
                     (degree % 90 == 0) ? Color{255, 230, 80, 255} : RAYWHITE);
      }
    }
  }

  EndScissorMode();

  const char *headingStr = TextFormat("%03.0f°", headingDeg);
  float headW = MeasureUIString(headingStr, 12.0f);
  Rectangle badgeRect = {centerX - (headW + 20) / 2.0f, startY - 12.0f,
                         headW + 20.0f, 18.0f};
  DrawRectangleRounded(badgeRect, 0.6f, 6, Color{10, 22, 36, 230});
  DrawRectangleRoundedLines(badgeRect, 0.6f, 6, Color{255, 215, 0, 240});
  DrawUIString(headingStr, badgeRect.x + 10.0f, badgeRect.y + 2.0f, 12.0f,
               Color{255, 215, 0, 255});
}

void BudgetFlightSimulatorScene::DrawAltitudeBar(float altitudeM,
                                                 float groundHeightM,
                                                 int screenWidth,
                                                 int screenHeight) {
  const float barWidth = 84.0f;
  const float barHeight = 220.0f;
  const float startX = screenWidth - barWidth - 28.0f;
  const float startY = screenHeight - barHeight - 48.0f;
  const float centerY = startY + barHeight / 2.0f;

  Rectangle altRect = {startX, startY, barWidth, barHeight};
  DrawGlassPanel(altRect, 0.18f, Color{14, 26, 42, 175});

  Rectangle headerRect = {startX - 2.0f, startY - 26.0f, barWidth + 4.0f,
                          22.0f};
  DrawGlassPanel(headerRect, 0.5f, Color{18, 38, 62, 220});
  const char *altVal = TextFormat("%.0f M", altitudeM);
  float altValW = MeasureUIString(altVal, 13.0f);
  DrawUIString(altVal, startX + (barWidth - altValW) / 2.0f, startY - 22.0f,
               13.0f, Color{130, 240, 255, 255});

  const float pxPerMeter = 1.4f;
  const float visibleM = (barHeight / 2.0f) / pxPerMeter;

  BeginScissorMode((int)startX + 4, (int)startY + 4, (int)barWidth - 8,
                   (int)barHeight - 8);

  int startStep = ((int)(altitudeM - visibleM) / 20) * 20;
  int endStep = ((int)(altitudeM + visibleM) / 20) * 20 + 20;

  for (int markM = startStep; markM <= endStep; markM += 20) {
    if (markM < 0)
      continue;
    float y = centerY - (markM - altitudeM) * pxPerMeter;
    bool isMajor = (markM % 100 == 0);

    float tickLen = isMajor ? 20.0f : 10.0f;
    DrawLineEx(Vector2{startX + 6.0f, y}, Vector2{startX + 6.0f + tickLen, y},
               2.0f, Color{120, 235, 255, 220});

    if (isMajor) {
      DrawUIString(TextFormat("%d", markM), startX + 30.0f, y - 6.0f, 10.0f,
                   RAYWHITE);
    }
  }

  float groundScreenY = centerY - (groundHeightM - altitudeM) * pxPerMeter;
  if (groundScreenY < startY + barHeight) {
    float h = (startY + barHeight) - groundScreenY;
    DrawRectangleRounded(
        Rectangle{startX + 4.0f, groundScreenY, barWidth - 8.0f, h}, 0.1f, 4,
        Fade(Color{230, 60, 40, 255}, 0.45f));
    DrawLineEx(Vector2{startX + 2.0f, groundScreenY},
               Vector2{startX + barWidth - 2.0f, groundScreenY}, 2.5f, RED);
  }

  EndScissorMode();

  DrawTriangle(
      Vector2{startX + 6.0f, centerY}, Vector2{startX + 16.0f, centerY - 6.0f},
      Vector2{startX + 16.0f, centerY + 6.0f}, Color{255, 215, 0, 255});
}

void BudgetFlightSimulatorScene::DrawPitchLadderHUD(float pitchDeg,
                                                    float rollDeg,
                                                    int screenHeight) {
  const float boxW = 160.0f;
  const float boxH = 160.0f;
  const float posX = 24.0f;
  const float posY = screenHeight - boxH - 48.0f;
  const float cx = posX + boxW / 2.0f;
  const float cy = posY + boxH / 2.0f;

  Rectangle pitchRect = {posX, posY, boxW, boxH};
  DrawGlassPanel(pitchRect, 0.22f, Color{14, 26, 42, 175});

  DrawUIString("ATTITUDE", posX + 12.0f, posY + 8.0f, 11.0f,
               Color{255, 215, 0, 255});
  DrawUIString(TextFormat("P: %+.1f°", pitchDeg), posX + 12.0f, posY + 23.0f,
               11.0f, Color{100, 240, 255, 255});
  DrawUIString(TextFormat("R: %+.1f°", rollDeg), posX + 88.0f, posY + 23.0f,
               11.0f, Color{100, 240, 255, 255});

  BeginScissorMode((int)posX + 6, (int)posY + 38, (int)boxW - 12,
                   (int)boxH - 44);

  float rad = rollDeg * DEG2RAD;
  float cosR = cosf(rad);
  float sinR = sinf(rad);
  float pxPerDeg = 2.2f;

  for (int p = -80; p <= 80; p += 10) {
    if (p == 0)
      continue;
    float dy = (pitchDeg - (float)p) * pxPerDeg;
    float barLen = (p % 20 == 0) ? 38.0f : 24.0f;
    Color barCol =
        (p > 0) ? Color{100, 245, 140, 230} : Color{90, 190, 255, 230};

    Vector2 l1 = {cx + (-barLen * cosR - dy * -sinR),
                  cy + (-barLen * sinR + dy * cosR)};
    Vector2 l2 = {cx + (-8.0f * cosR - dy * -sinR),
                  cy + (-8.0f * sinR + dy * cosR)};
    DrawLineEx(l1, l2, 2.0f, barCol);

    Vector2 r1 = {cx + (8.0f * cosR - dy * -sinR),
                  cy + (8.0f * sinR + dy * cosR)};
    Vector2 r2 = {cx + (barLen * cosR - dy * -sinR),
                  cy + (barLen * sinR + dy * cosR)};
    DrawLineEx(r1, r2, 2.0f, barCol);
  }

  float horDy = pitchDeg * pxPerDeg;
  Vector2 hLeft = {cx + (-55.0f * cosR - horDy * -sinR),
                   cy + (-55.0f * sinR + horDy * cosR)};
  Vector2 hRight = {cx + (55.0f * cosR - horDy * -sinR),
                    cy + (55.0f * sinR + horDy * cosR)};
  DrawLineEx(hLeft, hRight, 2.5f, WHITE);

  EndScissorMode();

  DrawCircle((int)cx, (int)cy, 3.0f, Color{255, 215, 0, 255});
  DrawLineEx(Vector2{cx - 12.0f, cy}, Vector2{cx - 4.0f, cy}, 2.0f,
             Color{255, 215, 0, 255});
  DrawLineEx(Vector2{cx + 4.0f, cy}, Vector2{cx + 12.0f, cy}, 2.0f,
             Color{255, 215, 0, 255});
  DrawLineEx(Vector2{cx, cy - 8.0f}, Vector2{cx, cy - 3.0f}, 2.0f,
             Color{255, 215, 0, 255});
}

void BudgetFlightSimulatorScene::DrawTelemetryCluster(int screenWidth,
                                                      int screenHeight) {
  const float posX = 24.0f;
  const float boxW = 270.0f;
  const float boxH = 145.0f;
  const float posY = screenHeight - 160.0f - 48.0f - boxH - 12.0f;

  Rectangle panelRect = {posX, posY, boxW, boxH};
  DrawGlassPanel(panelRect, 0.18f, Color{14, 26, 42, 175});

  DrawUIString(m_aircraft.stats.name.c_str(), posX + 14.0f, posY + 10.0f, 15.0f,
               Color{255, 215, 0, 255});

  DrawUIString(TextFormat("AIRSPEED   %4.0f KM/H", m_aircraft.GetAirspeedKmh()),
               posX + 14.0f, posY + 32.0f, 12.0f, Color{120, 240, 255, 255});
  DrawUIString(TextFormat("ALTITUDE   %4.1f M", m_aircraft.GetAltitudeMeters()),
               posX + 14.0f, posY + 50.0f, 12.0f, Color{120, 240, 255, 255});

  DrawUIString("THR", posX + 14.0f, posY + 74.0f, 10.0f, RAYWHITE);
  Rectangle thrBg = {posX + 48.0f, posY + 72.0f, 140.0f, 13.0f};
  DrawRectangleRounded(thrBg, 0.6f, 4, Color{20, 40, 60, 180});
  DrawRectangleRounded(
      Rectangle{thrBg.x, thrBg.y, thrBg.width * m_throttle, thrBg.height}, 0.6f,
      4, Color{80, 210, 255, 255});
  DrawRectangleRoundedLines(thrBg, 0.6f, 4, Color{130, 220, 255, 200});
  DrawUIString(TextFormat("%.0f%%", m_throttle * 100.0f), posX + 196.0f,
               posY + 72.0f, 11.0f, Color{120, 240, 255, 255});

  float hpNorm = Clamp(m_aircraft.health / 100.0f, 0.0f, 1.0f);
  Color hpCol = (hpNorm > 0.5f) ? Color{100, 245, 140, 255}
                                : (hpNorm > 0.25f ? Color{255, 215, 0, 255}
                                                  : Color{255, 75, 75, 255});
  DrawUIString("HULL", posX + 14.0f, posY + 94.0f, 10.0f, RAYWHITE);
  Rectangle hpBg = {posX + 48.0f, posY + 92.0f, 140.0f, 13.0f};
  DrawRectangleRounded(hpBg, 0.6f, 4, Color{20, 40, 60, 180});
  DrawRectangleRounded(
      Rectangle{hpBg.x, hpBg.y, hpBg.width * hpNorm, hpBg.height}, 0.6f, 4,
      hpCol);
  DrawRectangleRoundedLines(hpBg, 0.6f, 4, Color{130, 220, 255, 200});
  DrawUIString(TextFormat("%.0f%%", m_aircraft.health), posX + 196.0f,
               posY + 92.0f, 11.0f, hpCol);

  const char *status =
      m_engineFailed
          ? "!! ENGINE FLAMEOUT / DEADSTICK !!"
          : (m_aircraft.isOnGround
                 ? "ON RUNWAY / TAXI"
                 : (m_aircraft.isStalling ? "STALL WARNING!" : "IN FLIGHT"));

  Color statusCol =
      m_engineFailed
          ? Color{255, 45, 45, 255}
          : (m_aircraft.isOnGround
                 ? RAYWHITE
                 : (m_aircraft.isStalling ? Color{255, 70, 70, 255}
                                          : Color{100, 245, 140, 255}));

  DrawUIString(status, posX + 14.0f, posY + 118.0f, 12.0f, statusCol);

  float t = m_engine->GetDayNightCycle().GetTime();
  int hours = (int)t;
  int minutes = (int)((t - (float)hours) * 60.0f);
  auto &weatherSys = m_engine->GetWeatherSystem();
  std::string envStr = TextFormat("%02d:%02d | %s", hours, minutes,
                                  weatherSys.GetCurrentWeatherName().c_str());
  float envW = MeasureUIString(envStr.c_str(), 12.0f);

  Rectangle envRect = {(float)(screenWidth - envW - 46.0f), 16.0f, envW + 30.0f,
                       32.0f};
  DrawGlassPanel(envRect, 0.5f, Color{14, 26, 42, 175});
  DrawUIString(envStr.c_str(), envRect.x + 15.0f, envRect.y + 9.0f, 12.0f,
               Color{120, 240, 255, 255});
}

void BudgetFlightSimulatorScene::Draw() {
  int screenWidth = GetScreenWidth();
  int screenHeight = GetScreenHeight();

  DrawCompassTape(m_aircraft.GetHeading(), screenWidth);
  DrawRadarHUD(screenWidth, screenHeight);
  DrawTelemetryCluster(screenWidth, screenHeight);
  DrawPitchLadderHUD(m_aircraft.GetPitchAngle(), m_aircraft.GetRollAngle(),
                     screenHeight);

  float groundH =
      m_terrain.GetHeightAt(m_aircraft.position.x, m_aircraft.position.z);
  DrawAltitudeBar(m_aircraft.GetAltitudeMeters(), groundH, screenWidth,
                  screenHeight);

  if (m_explosionPostFXTimer > 0.05f) {
    DrawRectangle(0, 0, screenWidth, screenHeight,
                  Fade(RED, m_explosionPostFXTimer * 0.4f));
  }

  if (m_aircraft.stats.hasWeapons) {
    int cx = screenWidth / 2;
    int cy = screenHeight / 2;

    DrawCircleLines(cx, cy, 22, Fade(Color{120, 240, 255, 255}, 0.75f));
    DrawCircleLines(cx, cy, 3, Fade(Color{120, 240, 255, 255}, 0.95f));

    DrawLineEx(Vector2{(float)cx - 30.0f, (float)cy},
               Vector2{(float)cx - 12.0f, (float)cy}, 2.0f,
               Color{120, 240, 255, 255});
    DrawLineEx(Vector2{(float)cx + 12.0f, (float)cy},
               Vector2{(float)cx + 30.0f, (float)cy}, 2.0f,
               Color{120, 240, 255, 255});
    DrawLineEx(Vector2{(float)cx, (float)cy - 30.0f},
               Vector2{(float)cx, (float)cy - 12.0f}, 2.0f,
               Color{120, 240, 255, 255});
    DrawLineEx(Vector2{(float)cx, (float)cy + 12.0f},
               Vector2{(float)cx, (float)cy + 30.0f}, 2.0f,
               Color{120, 240, 255, 255});

    const float altBarW = 84.0f;
    float ammoBoxW = 240.0f;
    float ammoBoxH = 74.0f;
    float ammoBoxX = (float)screenWidth - altBarW - ammoBoxW - 48.0f;
    Rectangle wpnRect = {ammoBoxX, (float)(screenHeight - 110), ammoBoxW,
                         ammoBoxH};
    DrawGlassPanel(wpnRect, 0.25f, Color{14, 26, 42, 175});

    DrawUIString("WEAPONS LOADOUT", wpnRect.x + 14.0f, wpnRect.y + 10.0f, 12.0f,
                 Color{255, 215, 0, 255});
    DrawUIString("GUN:   20MM VULCAN [SPACE]", wpnRect.x + 14.0f,
                 wpnRect.y + 30.0f, 11.0f, Color{100, 245, 140, 255});
    DrawUIString(TextFormat("MISSILE: %d / %d [LCTRL]", m_aircraft.missileAmmo,
                            m_aircraft.stats.maxMissiles),
                 wpnRect.x + 14.0f, wpnRect.y + 48.0f, 11.0f,
                 m_aircraft.missileAmmo > 0 ? Color{255, 150, 50, 255}
                                            : Color{255, 75, 75, 255});

    for (const auto &enemy : m_enemyJets) {
      if (enemy.isAlive) {
        Vector2 screenPos =
            GetWorldToScreen(enemy.position, m_engine->GetCamera());
        Vector3 toEnemy = Vector3Subtract(enemy.position, m_aircraft.position);
        Vector3 camFwd = Vector3Normalize(Vector3Subtract(
            m_engine->GetCamera().target, m_engine->GetCamera().position));

        if (Vector3DotProduct(toEnemy, camFwd) > 0.0f) {
          float distM = fmaxf(Vector3Length(toEnemy), 50.0f);
          float boxSize = Clamp((1400.0f / distM) * 18.0f, 26.0f, 90.0f);
          float halfBox = boxSize * 0.5f;

          DrawRectangleLines((int)(screenPos.x - halfBox),
                             (int)(screenPos.y - halfBox), (int)boxSize,
                             (int)boxSize, RED);
          DrawUIString("LOCK-ON", screenPos.x - 28.0f,
                       screenPos.y - halfBox - 16.0f, 12.0f, RED);
          DrawUIString(TextFormat("%.0fm", distM), screenPos.x - 20.0f,
                       screenPos.y + halfBox + 4.0f, 12.0f, RED);
        }
      }
    }
  }

  // Mission HUD
  const MissionData *curMission = m_missionManager.GetCurrentMission();
  if (curMission) {
    const float missionBoxW = 380.0f;
    const int contentInnerW = (int)missionBoxW - 28;

    std::string mObj =
        TextFormat("OBJECTIVE: %s", curMission->description.c_str());

    int estimatedLines = 1;
    std::string tempLine = "";
    std::string tempWord = "";
    for (size_t i = 0; i <= mObj.length(); ++i) {
      char c = (i < mObj.length()) ? mObj[i] : ' ';
      if (c == ' ' || i == mObj.length()) {
        if (!tempWord.empty()) {
          std::string test =
              tempLine.empty() ? tempWord : (tempLine + " " + tempWord);
          if (MeasureUIString(test.c_str(), 11.0f) > (float)contentInnerW &&
              !tempLine.empty()) {
            estimatedLines++;
            tempLine = tempWord;
          } else {
            tempLine = test;
          }
          tempWord.clear();
        }
      } else {
        tempWord += c;
      }
    }

    float calculatedH = 46.0f + (estimatedLines * 16.0f);
    Rectangle missionRect = {20.0f, 16.0f, missionBoxW, calculatedH};
    DrawGlassPanel(missionRect, 0.22f, Color{14, 26, 42, 185});

    std::string mTitle = TextFormat("MISSION: %s", curMission->title.c_str());
    DrawUIString(mTitle.c_str(), missionRect.x + 14.0f, missionRect.y + 12.0f,
                 13.0f, Color{255, 215, 0, 255});

    DrawTextWordWrap(mObj.c_str(), (int)missionRect.x + 14,
                     (int)missionRect.y + 34, contentInnerW, 11, 4, RAYWHITE);
  }

  const char *bottomBarText =
      "Planes: [ [ ] / [ ] ] | Missions: [ N / M ] | Drop: [ G ] | Menu: [ "
      "ESC ]";
  float bbW = MeasureUIString(bottomBarText, 11.0f);
  Rectangle bbRect = {(float)(screenWidth / 2.0f - bbW / 2.0f - 16.0f),
                      (float)(screenHeight - 30), bbW + 32.0f, 22.0f};
  DrawGlassPanel(bbRect, 0.5f, Color{12, 22, 36, 210});
  DrawUIString(bottomBarText, screenWidth / 2.0f - bbW / 2.0f,
               screenHeight - 26, 11.0f, Color{210, 240, 255, 240});

  if (m_aircraft.isStalling) {
    float stallW = MeasureUIString("!! STALL WARNING !!", 22.0f);
    DrawUIString("!! STALL WARNING !!", screenWidth / 2.0f - stallW / 2.0f,
                 80.0f, 22.0f, RED);
  }
}

void BudgetFlightSimulatorScene::Shutdown() {
  if (m_customFontLoaded) {
    UnloadFont(m_hudFont);
    m_customFontLoaded = false;
  }

  for (auto &pair : m_loadedAircraftModels) {
    UnloadModel(pair.second);
  }
  m_loadedAircraftModels.clear();
  m_modelLoaded = false;

  if (m_enemyJetModelLoaded) {
    UnloadModel(m_enemyJetModel);
    m_enemyJetModelLoaded = false;
  }
  if (m_cargoModelLoaded) {
    UnloadModel(m_cargoModel);
    m_cargoModelLoaded = false;
  }
  if (m_particlesInitialized) {
    UnloadTexture(m_particleTexture);
    m_particlesInitialized = false;
  }
  m_ocean.Unload();
}
