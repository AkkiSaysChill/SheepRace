#include "game/BudgetFlightSimulatorScene.h"
#include "engine/DrawableComponent.h"
#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "game/AircraftDatabase.h"
#include "game/FlightSession.h"
#include "game/MissionDatabase.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cstdio>
#include <vector>
#include <algorithm>

BudgetFlightSimulatorScene::BudgetFlightSimulatorScene() : m_engine(nullptr) {}

BudgetFlightSimulatorScene::~BudgetFlightSimulatorScene() { Shutdown(); }

void BudgetFlightSimulatorScene::SwitchAircraft(const std::string &planeId) {
  AircraftStats newStats;
  if (!AircraftDatabase::GetInstance().GetStats(planeId, newStats))
    return;

  m_aircraft.SetStats(newStats);

  auto &assetLoader = m_engine->GetAssetLoader();
  auto &renderer = m_engine->GetRenderer();
  Shader shadowShader = renderer.GetShadowShader();

  shadowShader.locs[SHADER_LOC_COLOR_DIFFUSE] =
      GetShaderLocation(shadowShader, "colDiffuse");
  shadowShader.locs[SHADER_LOC_MAP_ALBEDO] =
      GetShaderLocation(shadowShader, "texture0");

  if (m_modelLoaded) {
    UnloadModel(m_planeModel);
    m_modelLoaded = false;
  }

  auto planeData = assetLoader.getAssetData(newStats.modelFile);
  if (!planeData.empty()) {
    m_planeModel = assetLoader.LoadModelFromAssetData(planeData, ".glb");
    m_modelLoaded = true;

    for (int i = 0; i < m_planeModel.materialCount; i++) {
      m_planeModel.materials[i].shader = shadowShader;
    }
  } else {
    m_planeModel = LoadModelFromMesh(GenMeshCube(1.5f, 0.5f, 4.0f));
    m_planeModel.materials[0].shader = shadowShader;
    m_planeModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = GRAY;
    m_modelLoaded = true;
  }

  BoundingBox modelBounds = GetModelBoundingBox(m_planeModel);
  m_flightCamera.AutoAdjustForModel(modelBounds);

  if (m_planeObject) {
    auto drawable = m_planeObject->GetComponent<DrawableComponent>();
    if (drawable) {
      drawable->GetModelRef() = m_planeModel;
    } else {
      m_planeObject->AddComponent<DrawableComponent>(m_planeModel);
    }
  }
}

void BudgetFlightSimulatorScene::Initialize(Engine *engine) {
  Scene3D::Initialize(engine);
  m_engine = engine;

  m_terrain.LoadWorld(m_engine, "island_world.glb");
  m_ocean.Initialize(m_engine, 260000.0f);

  m_aircraft.Reset({-4850.0f, 111.5f, 150.0f});
  m_throttle = 0.0f;

  m_flightCamera.Initialize(55.0f, 15.0f);

  auto &gameObjectManager = m_engine->GetGameObjectManager();
  auto &assetLoader = m_engine->GetAssetLoader();

  assetLoader.loadPackedFile("shaders.bin");
  assetLoader.loadPackedFile("planes.woosh");
  assetLoader.loadPackedFile("effects.dyum");

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

  m_planeObject = std::make_shared<GameObject>();
  m_planeObject->name = "PlayerAircraft";
  m_planeObject->position = m_aircraft.position;
  gameObjectManager.AddGameObject(m_planeObject);

  auto xmlData = assetLoader.getAssetData("aircraft.xml");
  if (!xmlData.empty()) {
    AircraftDatabase::GetInstance().LoadFromMemory(xmlData.data(),
                                                   xmlData.size());
  } else {
    AircraftDatabase::GetInstance().LoadFromFile("assets/data/aircraft.xml");
  }

  std::string startingPlane = FlightSession::GetInstance().selectedPlaneId;
  const auto &ids = AircraftDatabase::GetInstance().GetAllPlaneIds();
  if (!ids.empty()) {
    m_currentPlaneIndex = 0;
    SwitchAircraft(startingPlane.empty() ? ids[0] : startingPlane);
  }

  // Hostile Ace Jet Model (Clone of jet_mk2 tinted red)
  auto enemyJetData = assetLoader.getAssetData("jet_mk2.glb");
  if (!enemyJetData.empty()) {
      m_enemyJetModel = assetLoader.LoadModelFromAssetData(enemyJetData, ".glb");
      Shader shadowShader = m_engine->GetRenderer().GetShadowShader();
      for (int i = 0; i < m_enemyJetModel.materialCount; i++) {
          m_enemyJetModel.materials[i].shader = shadowShader;
          m_enemyJetModel.materials[i].maps[MATERIAL_MAP_ALBEDO].color = Color{ 255, 65, 65, 255 };
      }
      m_enemyJetModelLoaded = true;
  }

  // Cargo Box Model
  m_cargoModel = LoadModelFromMesh(GenMeshCube(4.0f, 4.0f, 4.0f));
  m_cargoModel.materials[0].shader = m_engine->GetRenderer().GetShadowShader();
  m_cargoModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = ORANGE;
  m_cargoModelLoaded = true;

  // -------------------------------------------------------------
  // PARTICLE SYSTEM SETUP
  // -------------------------------------------------------------
  Image whitePixel = GenImageColor(8, 8, WHITE);
  m_particleTexture = LoadTextureFromImage(whitePixel);
  UnloadImage(whitePixel);

  auto partVsData = assetLoader.getAssetData("instanced_particle.vs");
  auto partFsData = assetLoader.getAssetData("instanced_particle.fs");
  if (!partVsData.empty() && !partFsData.empty()) {
      std::string vs(partVsData.begin(), partVsData.end());
      std::string fs(partFsData.begin(), partFsData.end());
      m_particleShader = LoadShaderFromMemory(vs.c_str(), fs.c_str());
      m_particleSystem.Initialize(m_particleTexture, m_particleShader);
      m_particlesInitialized = true;

      // 1. Detonation / Explosion Emitter
      EmitterConfig expCfg;
      expCfg.maxParticles = 800;
      expCfg.emissionMode = EmissionMode::BURST;
      expCfg.lifetimeRange = { 0.4f, 0.85f };
      expCfg.speedRange = { 12.0f, 65.0f };
      expCfg.startSize = 3.5f;
      expCfg.endSize = 0.2f;
      expCfg.startColor = Color{ 255, 215, 60, 255 };
      expCfg.endColor   = Color{ 255, 45, 10, 0 };
      expCfg.drag = 1.25f;
      expCfg.gravity = { 0.0f, -4.0f, 0.0f };
      expCfg.randomRotation = true;
      m_explosionEmitter = m_particleSystem.AddEmitter(expCfg);

      // 2. Missile Exhaust / Smoke Trail Emitter
      EmitterConfig smokeCfg;
      smokeCfg.maxParticles = 1200;
      smokeCfg.emissionMode = EmissionMode::BURST;
      smokeCfg.lifetimeRange = { 0.25f, 0.60f };
      smokeCfg.speedRange = { 1.0f, 8.0f };
      smokeCfg.startSize = 1.4f;
      smokeCfg.endSize = 2.8f;
      smokeCfg.startColor = Color{ 255, 140, 40, 220 };
      smokeCfg.endColor   = Color{ 90, 90, 90, 0 };
      smokeCfg.drag = 0.8f;
      smokeCfg.randomRotation = true;
      m_missileSmokeEmitter = m_particleSystem.AddEmitter(smokeCfg);

      // 3. Bullet Spark / Impact Emitter
      EmitterConfig sparkCfg;
      sparkCfg.maxParticles = 500;
      sparkCfg.emissionMode = EmissionMode::BURST;
      sparkCfg.lifetimeRange = { 0.10f, 0.35f };
      sparkCfg.speedRange = { 10.0f, 40.0f };
      sparkCfg.startSize = 0.8f;
      sparkCfg.endSize = 0.05f;
      sparkCfg.startColor = Color{ 255, 255, 200, 255 };
      sparkCfg.endColor   = Color{ 255, 120, 20, 0 };
      sparkCfg.drag = 2.0f;
      m_bulletSparkEmitter = m_particleSystem.AddEmitter(sparkCfg);
  }

  // -------------------------------------------------------------
  // MISSION INITIALIZATION
  // -------------------------------------------------------------
  assetLoader.loadPackedFile("DATA/MISSIONS/missions.bin");
  auto missionBin = assetLoader.getAssetData("missions.xml");

  if (!missionBin.empty()) {
      MissionDatabase::GetInstance().LoadFromMemory(missionBin.data(), missionBin.size());
  } else {
      MissionDatabase::GetInstance().LoadFromFile("DATA/MISSIONS/missions.xml");
  }

  m_missionManager.Initialize(m_engine);

  m_missionManager.SetSetupCallback([this](const std::string& planeId, Vector3 spawnPos, float headingDeg) {
      if (!planeId.empty()) {
          SwitchAircraft(planeId);
      }
      m_aircraft.Reset(spawnPos);

      // Raylib yaw is inverted: negate headingDeg so 90 deg points East
      m_aircraft.orientation = QuaternionFromEuler(0.0f, -headingDeg * DEG2RAD, 0.0f);
      m_throttle = 0.0f;

      m_droppedPayloads.clear();
      m_bullets.clear();
      m_missiles.clear();
      m_enemyJets.clear();

      if (m_missionManager.GetCurrentMission() && 
          m_missionManager.GetCurrentMission()->id == "canyon_strike_02") {
          EnemyAce ace;
          ace.id = "hostile_ace";
          ace.name = "Crimson Phantom Ace";
          ace.position = { 1800.0f, 480.0f, 1400.0f };
          ace.orientation = QuaternionFromEuler(0.0f, 180.0f * DEG2RAD, 0.0f);
          ace.velocity = { 0.0f, 0.0f, -140.0f };
          ace.health = 150.0f;
          ace.maxHealth = 150.0f;
          ace.hitRadius = 14.0f;
          ace.isAlive = true;
          ace.gunCooldown = 0.0f;
          ace.missileCooldown = 3.0f;
          m_enemyJets.push_back(ace);
      }
  });

  m_missionManager.SetEventCallback([this](const std::string& eventName, const Vector3& stagePos) {
      if (eventName == "DROP_CARGO") {
          DroppedPayload p;
          p.position = Vector3Subtract(m_aircraft.position, Vector3{0, 2.5f, 0});
          p.velocity = m_aircraft.velocity;
          p.active = true;
          p.inWater = false;
          m_droppedPayloads.push_back(p);
      } else if (eventName == "ENGINE_FAIL") {
          m_throttle = 0.0f;
      }
  });

  auto missionIds = MissionDatabase::GetInstance().GetAllMissionIds();
  if (!missionIds.empty()) {
      m_missionManager.StartMission(missionIds[0]);
  }
}

void BudgetFlightSimulatorScene::UpdateDogfight(float deltaTime) {
    for (auto& enemy : m_enemyJets) {
        if (!enemy.isAlive) continue;

        if (enemy.gunCooldown > 0.0f) enemy.gunCooldown -= deltaTime;
        if (enemy.missileCooldown > 0.0f) enemy.missileCooldown -= deltaTime;

        Vector3 toPlayer = Vector3Subtract(m_aircraft.position, enemy.position);
        float distToPlayer = Vector3Length(toPlayer);
        Vector3 desiredFwd = Vector3Normalize(toPlayer);

        Vector3 enemyFwd = Vector3RotateByQuaternion(Vector3{0, 0, -1}, enemy.orientation);
        Vector3 newFwd = Vector3Normalize(Vector3Lerp(enemyFwd, desiredFwd, 2.2f * deltaTime));

        Vector3 rotAxis = Vector3Normalize(Vector3CrossProduct(enemyFwd, newFwd));
        float angleDiff = acosf(Clamp(Vector3DotProduct(enemyFwd, newFwd), -1.0f, 1.0f));
        if (angleDiff > 0.001f && Vector3Length(rotAxis) > 0.1f) {
            enemy.orientation = QuaternionNormalize(
                QuaternionMultiply(enemy.orientation, QuaternionFromAxisAngle(rotAxis, angleDiff * 3.5f * deltaTime))
            );
        }

        enemy.velocity = Vector3Scale(newFwd, 140.0f);
        enemy.position = Vector3Add(enemy.position, Vector3Scale(enemy.velocity, deltaTime));

        float forwardAlignment = Vector3DotProduct(newFwd, desiredFwd);

        if (forwardAlignment > 0.94f && distToPlayer < 600.0f && enemy.gunCooldown <= 0.0f) {
            enemy.gunCooldown = 0.09f;
            BulletTracer b;
            b.position = Vector3Add(enemy.position, Vector3Scale(newFwd, 4.0f));
            b.velocity = Vector3Add(Vector3Scale(newFwd, 1000.0f), enemy.velocity);
            b.life = 1.4f;
            b.isEnemy = true;
            b.active = true;
            m_bullets.push_back(b);
        }

        if (forwardAlignment > 0.85f && distToPlayer > 350.0f && distToPlayer < 1200.0f && enemy.missileCooldown <= 0.0f) {
            enemy.missileCooldown = 6.0f;

            HomingMissile m;
            m.position = Vector3Subtract(enemy.position, Vector3{0, 1.5f, 0});
            m.velocity = Vector3Scale(newFwd, 150.0f);
            m.targetPos = m_aircraft.position;
            m.isTrackingPlayer = true;
            m.speed = 310.0f;
            m.turnRate = 2.4f;
            m.life = 6.5f;
            m.blastRadius = 35.0f;
            m.damage = 45.0f;
            m.active = true;
            m_missiles.push_back(m);
        }
    }
}

void BudgetFlightSimulatorScene::UpdateProjectiles(float deltaTime) {
    for (auto& b : m_bullets) {
        if (!b.active) continue;

        b.life -= deltaTime;
        if (b.life <= 0.0f) {
            b.active = false;
            continue;
        }

        b.position = Vector3Add(b.position, Vector3Scale(b.velocity, deltaTime));

        if (!b.isEnemy) {
            for (auto& enemy : m_enemyJets) {
                if (!enemy.isAlive) continue;
                if (Vector3Distance(b.position, enemy.position) <= enemy.hitRadius) {
                    b.active = false;
                    enemy.health -= m_aircraft.stats.bulletDamage;
                    if (m_bulletSparkEmitter) m_bulletSparkEmitter->Emit(15, b.position);

                    if (enemy.health <= 0.0f) {
                        enemy.isAlive = false;
                        if (m_explosionEmitter) m_explosionEmitter->Emit(120, enemy.position);
                    }
                    break;
                }
            }
        } else {
            if (Vector3Distance(b.position, m_aircraft.position) <= 8.0f) {
                b.active = false;
                m_aircraft.health -= 6.0f;
                if (m_bulletSparkEmitter) m_bulletSparkEmitter->Emit(18, b.position);
            }
        }
    }

    m_bullets.erase(std::remove_if(m_bullets.begin(), m_bullets.end(), 
        [](const BulletTracer& b) { return !b.active; }), m_bullets.end());

    for (auto& m : m_missiles) {
        if (!m.active) continue;

        m.life -= deltaTime;
        if (m.life <= 0.0f) {
            m.active = false;
            if (m_explosionEmitter) m_explosionEmitter->Emit(40, m.position);
            continue;
        }

        if (m_missileSmokeEmitter) m_missileSmokeEmitter->Emit(2, m.position);

        if (m.isTrackingPlayer) {
            m.targetPos = m_aircraft.position;
        } else {
            float closestDist = 99999.0f;
            for (const auto& enemy : m_enemyJets) {
                if (!enemy.isAlive) continue;
                float d = Vector3Distance(m.position, enemy.position);
                if (d < closestDist) {
                    closestDist = d;
                    m.targetPos = enemy.position;
                }
            }
        }

        Vector3 toTarget = Vector3Subtract(m.targetPos, m.position);
        float distToTarget = Vector3Length(toTarget);

        if (distToTarget > 1.0f) {
            Vector3 desiredDir = Vector3Normalize(toTarget);
            Vector3 currentDir = Vector3Normalize(m.velocity);
            Vector3 steerDir = Vector3Normalize(Vector3Lerp(currentDir, desiredDir, m.turnRate * deltaTime));
            m.velocity = Vector3Scale(steerDir, m.speed);
        }

        m.position = Vector3Add(m.position, Vector3Scale(m.velocity, deltaTime));

        if (m.isTrackingPlayer) {
            if (distToTarget <= 12.0f) {
                m.active = false;
                m_aircraft.health -= m.damage;
                if (m_explosionEmitter) m_explosionEmitter->Emit(150, m.position);
            }
        } else {
            for (auto& enemy : m_enemyJets) {
                if (!enemy.isAlive) continue;
                if (Vector3Distance(m.position, enemy.position) <= m.blastRadius) {
                    m.active = false;
                    enemy.health -= m.damage;
                    if (m_explosionEmitter) m_explosionEmitter->Emit(150, m.position);

                    if (enemy.health <= 0.0f) {
                        enemy.isAlive = false;
                        if (m_explosionEmitter) m_explosionEmitter->Emit(180, enemy.position);
                    }
                    break;
                }
            }
        }
    }

    m_missiles.erase(std::remove_if(m_missiles.begin(), m_missiles.end(), 
        [](const HomingMissile& m) { return !m.active; }), m_missiles.end());
}

void BudgetFlightSimulatorScene::Update(float deltaTime) {
  if (IsKeyPressed(KEY_F)) {
    m_isFrozen = !m_isFrozen;
  }

  if (IsKeyPressed(KEY_ESCAPE)) {
    m_engine->GetSceneManager().LoadScene("HangarScene");
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

  auto &weatherSys = m_engine->GetWeatherSystem();
  if (IsKeyPressed(KEY_L)) {
    weatherSys.CycleNextWeather(4.0f);
    weatherSys.ResetStateTimer();
  }
  if (IsKeyPressed(KEY_K)) {
      int prev = ((int)weatherSys.GetCurrentPreset() + (int)WeatherPreset::COUNT - 1) % (int)WeatherPreset::COUNT;
      weatherSys.SetWeather((WeatherPreset)prev, 3.5f);
      weatherSys.ResetStateTimer();
  }

  const auto& missionIds = MissionDatabase::GetInstance().GetAllMissionIds();
  static int currentMissionIndex = 0;
  if (!missionIds.empty()) {
      if (IsKeyPressed(KEY_N)) {
          currentMissionIndex = (currentMissionIndex + 1) % missionIds.size();
          m_missionManager.StartMission(missionIds[currentMissionIndex]);
      }
      if (IsKeyPressed(KEY_M)) {
          currentMissionIndex = (currentMissionIndex + (int)missionIds.size() - 1) % missionIds.size();
          m_missionManager.StartMission(missionIds[currentMissionIndex]);
      }
  }

  weatherSys.Update(deltaTime, m_flightCamera.GetRaylibCamera().position, m_aircraft.velocity);

  auto &dayNight = m_engine->GetDayNightCycle();
  dayNight.Update(deltaTime);

  if (IsKeyDown(KEY_P)) dayNight.SetTime(dayNight.GetTime() + 1.0f * deltaTime);
  if (IsKeyDown(KEY_O)) dayNight.SetTime(dayNight.GetTime() - 1.0f * deltaTime);

  Shader shadowShader = m_engine->GetRenderer().GetShadowShader();
  Shader skyShader    = m_engine->GetRenderer().GetSkyShader();

  dayNight.ApplyToShaders(shadowShader, skyShader);

  const auto& weatherProf = weatherSys.GetActiveProfile();
  Vector4 dimmedSun = Vector4Scale(dayNight.GetCurrentLighting().lightColor, weatherProf.sunDimFactor);
  int lightColLoc = GetShaderLocation(shadowShader, "lightColor");
  if (lightColLoc != -1) {
    SetShaderValue(shadowShader, lightColLoc, &dimmedSun, SHADER_UNIFORM_VEC4);
  }

  if (skyShader.id > 0) {
    int covLoc = GetShaderLocation(skyShader, "u_cloudCover");
    if (covLoc != -1) SetShaderValue(skyShader, covLoc, &weatherProf.cloudCover, SHADER_UNIFORM_FLOAT);
    int falLoc = GetShaderLocation(skyShader, "u_cloudFalloff");
    if (falLoc != -1) SetShaderValue(skyShader, falLoc, &weatherProf.cloudFalloff, SHADER_UNIFORM_FLOAT);
    int fogLoc = GetShaderLocation(skyShader, "u_fogDensity");
    if (fogLoc != -1) SetShaderValue(skyShader, fogLoc, &weatherProf.fogDensity, SHADER_UNIFORM_FLOAT);
    int horLoc = GetShaderLocation(skyShader, "u_horizonColor");
    if (horLoc != -1) {
      Vector3 blendedHor = Vector3Multiply(dayNight.GetCurrentLighting().equatorColor, weatherProf.horizonHazeTint);
      SetShaderValue(skyShader, horLoc, &blendedHor, SHADER_UNIFORM_VEC3);
    }
  }

  if (!m_isFrozen) {
    FlightInput input;
    if (IsKeyDown(KEY_W)) input.pitch = -1.0f;
    if (IsKeyDown(KEY_S)) input.pitch = 1.0f;
    if (IsKeyDown(KEY_A)) input.roll = -1.0f;
    if (IsKeyDown(KEY_D)) input.roll = 1.0f;
    if (IsKeyDown(KEY_Q)) input.yaw = 1.0f;
    if (IsKeyDown(KEY_E)) input.yaw = -1.0f;
    input.brake = IsKeyDown(KEY_SPACE) && !m_aircraft.stats.hasWeapons;

    if (IsKeyDown(KEY_LEFT_SHIFT))   m_throttle += 0.35f * deltaTime;
    if (IsKeyDown(KEY_LEFT_CONTROL) && !m_aircraft.stats.hasWeapons) m_throttle -= 0.35f * deltaTime;
    m_throttle = Clamp(m_throttle, 0.0f, 1.0f);
    input.throttle = m_throttle;

    m_aircraft.Update(input, deltaTime);
    m_aircraft.UpdateWeaponTimers(deltaTime);

    if (m_aircraft.stats.hasWeapons) {
        if (IsKeyDown(KEY_SPACE)) {
            m_aircraft.FireMinigun(m_bullets);
        }

        if (IsKeyPressed(KEY_LEFT_CONTROL)) {
            Vector3 lockTarget = Vector3Add(m_aircraft.position, Vector3Scale(m_aircraft.GetForward(), 1000.0f));
            for (const auto& enemy : m_enemyJets) {
                if (enemy.isAlive) {
                    lockTarget = enemy.position;
                    break;
                }
            }
            m_aircraft.FireMissile(m_missiles, lockTarget);
        }
    }
  }

  m_flightCamera.Update(m_aircraft, deltaTime);

  Vector3 skySun = Vector3Negate(dayNight.GetCurrentLighting().lightDir);
  m_ocean.Update(deltaTime, m_flightCamera.GetRaylibCamera(), skySun);

  if (m_planeObject) {
    m_planeObject->position = m_aircraft.position;
    m_planeObject->rotation = m_aircraft.orientation;

    auto drawable = m_planeObject->GetComponent<DrawableComponent>();
    if (drawable && m_modelLoaded) {
      drawable->GetModelRef() = m_planeModel;
    }
  }

  float terrainGroundHeight = m_terrain.GetHeightAt(m_aircraft.position.x, m_aircraft.position.z) + 1.5f;
  if (m_aircraft.position.y <= terrainGroundHeight) {
    m_aircraft.position.y = terrainGroundHeight;
    m_aircraft.isOnGround = true;
    if (m_aircraft.velocity.y < 0.0f) m_aircraft.velocity.y = 0.0f;
  }

  UpdateDogfight(deltaTime);
  UpdateProjectiles(deltaTime);

  for (auto& p : m_droppedPayloads) {
      if (!p.active) continue;
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
  for (const auto& enemy : m_enemyJets) {
      if (!enemy.isAlive) enemyEliminated = true;
  }

  // -------------------------------------------------------------
  // CAMERA & RENDERER SYNC (FIXES SCENE CAMERA CUTTING TO SEA)
  // -------------------------------------------------------------
  Camera &engineCam = m_engine->GetCamera();
  Camera3D flyCam = m_flightCamera.GetRaylibCamera();
  engineCam.position = flyCam.position;
  engineCam.target = flyCam.target;
  engineCam.up = flyCam.up;
  engineCam.fovy = flyCam.fovy;
  engineCam.projection = flyCam.projection;

  Shader currentPostShader = m_engine->GetPostProcessing().GetCurrentShader();
  Vector2 res = {(float)GetRenderWidth(), (float)GetRenderHeight()};
  int resLoc = GetShaderLocation(currentPostShader, "resolution");
  if (resLoc != -1) {
    SetShaderValue(currentPostShader, resLoc, &res, SHADER_UNIFORM_VEC2);
  }

  bool actionKeyPressed = IsKeyPressed(KEY_G);
  m_missionManager.Update(
      deltaTime, 
      m_aircraft.position, 
      m_aircraft.GetAirspeedKmh(), 
      m_aircraft.GetAltitudeMeters(), 
      m_aircraft.isOnGround,
      actionKeyPressed,
      enemyEliminated
  );
}

void BudgetFlightSimulatorScene::Draw3D() {
  rlEnableDepthTest();
  rlEnableDepthMask();
  rlEnableBackfaceCulling();

  rlMatrixMode(RL_PROJECTION);
  rlPushMatrix();
  rlLoadIdentity();
  double aspect = (double)GetScreenWidth() / (double)GetScreenHeight();
  double top = 2.0 * tan(m_flightCamera.GetRaylibCamera().fovy * 0.5 * DEG2RAD);
  double right = top * aspect;
  rlFrustum(-right, right, -top, top, 2.0, 15000.0);
  rlMatrixMode(RL_MODELVIEW);

  Camera3D flyCam = m_flightCamera.GetRaylibCamera();
  Shader shadowShader = m_engine->GetRenderer().GetShadowShader();

  m_terrain.Draw(shadowShader);
  m_terrain.DrawProps(flyCam);
  m_ocean.Draw();

  m_engine->GetWeatherSystem().Draw3D(flyCam.position);
  m_missionManager.Draw3D(flyCam);

  if (m_cargoModelLoaded) {
      for (const auto& p : m_droppedPayloads) {
          if (p.active) {
              DrawModel(m_cargoModel, p.position, 1.0f, WHITE);
              DrawCubeWires(p.position, 4.2f, 4.2f, 4.2f, RED);
          }
      }
  }

  if (m_enemyJetModelLoaded) {
      for (const auto& enemy : m_enemyJets) {
          if (enemy.isAlive) {
              Matrix enemyMat = MatrixMultiply(
                  QuaternionToMatrix(enemy.orientation),
                  MatrixTranslate(enemy.position.x, enemy.position.y, enemy.position.z)
              );
              for (int i = 0; i < m_enemyJetModel.meshCount; ++i) {
                  DrawMesh(m_enemyJetModel.meshes[i],
                           m_enemyJetModel.materials[m_enemyJetModel.meshMaterial[i]],
                           enemyMat);
              }
          }
      }
  }

  if (m_modelLoaded) {
    Matrix planeTransform = MatrixMultiply(
        QuaternionToMatrix(m_aircraft.orientation),
        MatrixTranslate(m_aircraft.position.x, m_aircraft.position.y,
                        m_aircraft.position.z));
    for (int i = 0; i < m_planeModel.meshCount; ++i) {
      DrawMesh(m_planeModel.meshes[i],
               m_planeModel.materials[m_planeModel.meshMaterial[i]],
               planeTransform);
    }
  }

  for (const auto& b : m_bullets) {
      if (b.active) {
          Vector3 tail = Vector3Subtract(b.position, Vector3Scale(Vector3Normalize(b.velocity), 6.0f));
          DrawLine3D(tail, b.position, b.isEnemy ? RED : YELLOW);
      }
  }

  for (const auto& m : m_missiles) {
      if (m.active) {
          Vector3 fwd = Vector3Normalize(m.velocity);
          DrawCylinderEx(m.position, Vector3Add(m.position, Vector3Scale(fwd, 4.0f)), 0.45f, 0.45f, 8, m.isTrackingPlayer ? RED : WHITE);
      }
  }

  rlDrawRenderBatchActive();

  if (m_particlesInitialized) {
      m_particleSystem.Draw(flyCam);
  }

  rlMatrixMode(RL_PROJECTION);
  rlPopMatrix();
  rlMatrixMode(RL_MODELVIEW);
}

void BudgetFlightSimulatorScene::Draw() {
  DrawRectangle(20, 20, 280, 182, Fade(BLACK, 0.6f));
  DrawRectangleLines(20, 20, 280, 182, LIME);

  DrawText(TextFormat("PLANE:    %s", m_aircraft.stats.name.c_str()), 35, 30, 16, YELLOW);
  DrawText(TextFormat("AIRSPEED: %.0f KM/H", m_aircraft.GetAirspeedKmh()), 35, 52, 18, LIME);
  DrawText(TextFormat("ALTITUDE: %.1f M", m_aircraft.GetAltitudeMeters()), 35, 74, 18, LIME);
  DrawText(TextFormat("HEADING:  %.0f DEG", m_aircraft.GetHeading()), 35, 96, 18, LIME);
  DrawText(TextFormat("THROTTLE: %.0f%%", m_throttle * 100.0f), 35, 118, 18, LIME);

  Color hullColor = (m_aircraft.health > 50.0f) ? LIME : (m_aircraft.health > 25.0f ? YELLOW : RED);
  DrawText(TextFormat("HULL:     %.0f%%", m_aircraft.health), 35, 140, 18, hullColor);

  float t = m_engine->GetDayNightCycle().GetTime();
  int hours = (int)t;
  int minutes = (int)((t - (float)hours) * 60.0f);
  DrawText(TextFormat("TIME:     %02d:%02d", hours, minutes), 35, 162, 18, LIME);

  if (m_aircraft.stats.hasWeapons) {
      int cx = GetScreenWidth() / 2;
      int cy = GetScreenHeight() / 2;

      // Minigun Crosshair
      DrawCircleLines(cx, cy, 22, Fade(LIME, 0.75f));
      DrawCircleLines(cx, cy, 3, Fade(LIME, 0.95f));

      // Corrected DrawLine: 5 arguments (startX, startY, endX, endY, color)
      DrawLine(cx - 30, cy, cx - 12, cy, LIME);
      DrawLine(cx + 12, cy, cx + 30, cy, LIME);
      DrawLine(cx, cy - 30, cx, cy - 12, LIME);
      DrawLine(cx, cy + 12, cx, cy + 30, LIME);

      DrawRectangle(GetScreenWidth() - 250, GetScreenHeight() - 110, 230, 90, Fade(BLACK, 0.65f));
      DrawRectangleLines(GetScreenWidth() - 250, GetScreenHeight() - 110, 230, 90, SKYBLUE);
      DrawText("WEAPONS SYSTEMS", GetScreenWidth() - 235, GetScreenHeight() - 100, 14, YELLOW);
      DrawText("GUN:   20MM VULCAN [SPACE]", GetScreenWidth() - 235, GetScreenHeight() - 80, 14, LIME);
      DrawText(TextFormat("MISSILE: %d / %d [LCTRL]", m_aircraft.missileAmmo, m_aircraft.stats.maxMissiles), 
               GetScreenWidth() - 235, GetScreenHeight() - 60, 14, m_aircraft.missileAmmo > 0 ? ORANGE : RED);

      for (const auto& enemy : m_enemyJets) {
          if (enemy.isAlive) {
              Vector2 screenPos = GetWorldToScreen(enemy.position, m_flightCamera.GetRaylibCamera());
              Vector3 toEnemy = Vector3Subtract(enemy.position, m_aircraft.position);
              Vector3 camFwd = Vector3Normalize(Vector3Subtract(m_flightCamera.GetRaylibCamera().target, m_flightCamera.GetRaylibCamera().position));

              if (Vector3DotProduct(toEnemy, camFwd) > 0.0f) {
                  DrawRectangleLines((int)screenPos.x - 24, (int)screenPos.y - 24, 48, 48, RED);
                  DrawText("LOCK-ON", (int)screenPos.x - 28, (int)screenPos.y - 42, 14, RED);
                  float distM = Vector3Length(toEnemy);
                  DrawText(TextFormat("%.0fm", distM), (int)screenPos.x - 20, (int)screenPos.y + 28, 14, RED);
              }
          }
      }
  }

  DrawText("Missions: [ N / M ] | Minigun: [ SPACE ] | Homing Missile: [ LCTRL ] | Drop: [ G ]",
           20, GetScreenHeight() - 25, 16, RAYWHITE);

  if (m_aircraft.isStalling) {
    DrawText("!! STALL WARNING !!", GetScreenWidth() / 2 - 130, 80, 26, RED);
  }

  m_missionManager.DrawHUD(m_flightCamera.GetRaylibCamera());
}

void BudgetFlightSimulatorScene::Shutdown() {
  if (m_modelLoaded) {
    UnloadModel(m_planeModel);
    m_modelLoaded = false;
  }
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
    UnloadShader(m_particleShader);
    m_particlesInitialized = false;
  }
  m_ocean.Unload();
}