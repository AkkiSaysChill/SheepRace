#include "game/WuShangClanScene.h"
#include "engine/DrawableComponent.h"
#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/PhysicsComponent.h"
#include "engine/PhysicsSystem.h"
#include "game/ChatSystem.h"
#include "game/enemy.h"
#include "game/weapon.h"
#include "iostream"
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"
#include <memory>

WuShangClanScene::WuShangClanScene() : m_engine(nullptr) {}

WuShangClanScene::~WuShangClanScene() {}

void WuShangClanScene::Initialize(Engine *engine) {

  Scene3D::Initialize(engine);
  m_engine = engine;

  m_logicEditor.Initialize(m_engine);

  auto &gameObjectManager = m_engine->GetGameObjectManager();
  auto &assetLoader = m_engine->GetAssetLoader();
  auto &renderer = m_engine->GetRenderer();

  assetLoader.loadPackedFile("levelDAT");
  assetLoader.loadPackedFile("shaders.bin");
  m_chatSystem.Initialize();

  std::vector<char> vsData =
      assetLoader.getAssetData("glsl330/instanced_particle.vs");
  std::vector<char> fsData =
      assetLoader.getAssetData("glsl330/instanced_particle.fs");

  SaveFileData("temp_vs.glsl", vsData.data(), vsData.size());
  SaveFileData("temp_fs.glsl", fsData.data(), fsData.size());
  Shader vfxShader = LoadShader("temp_vs.glsl", "temp_fs.glsl");
  remove("temp_vs.glsl");
  remove("temp_fs.glsl");

  Image img = GenImageColor(1, 1, WHITE);
  Texture2D particleTex = LoadTextureFromImage(img);
  UnloadImage(img);

  globalVFX.Initialize(particleTex, vfxShader);

  emitterSmoke = globalVFX.AddEmitter(ParticlePresets::Smoke());
  emitterSparks = globalVFX.AddEmitter(ParticlePresets::Sparks());
  emitterRain = globalVFX.AddEmitter(ParticlePresets::Rain());
  emitterSplash = globalVFX.AddEmitter(ParticlePresets::WaterSplash());
  emitterShockwave = globalVFX.AddEmitter(ParticlePresets::Shockwave());

  emitterRain->config.emissionRate = 0.0f;

  if (vfxShader.id == 0)
    printf("[VFX] FATAL: Shader failed to compile/link!\n");
  else
    printf("[VFX] Shader OK. id=%d\n", vfxShader.id);

  std::vector<char> shader_data =
      assetLoader.getAssetData("glsl330/alpha_discard.fs");

  Shader shadowShader = renderer.GetShadowShader();
  m_engine->PostProcessingActive = true;

  int ambientLoc = GetShaderLocation(shadowShader, "ambient");
  if (ambientLoc != -1) {
    float pitchBlack[4] = {0.05f, 0.05f, 0.05f, 1.0f};
    SetShaderValue(shadowShader, ambientLoc, pitchBlack, SHADER_UNIFORM_VEC4);
  }

  // Static Pillar
  auto staticData = assetLoader.getAssetData("pillar_static.glb");
  Model staticModel = assetLoader.LoadModelFromAssetData(staticData, ".glb");

  for (int i = 0; i < staticModel.materialCount; i++) {
    staticModel.materials[i].maps[MATERIAL_MAP_ALBEDO].color = GRAY;
    staticModel.materials[i].shader = shadowShader;
  }
  staticModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = LIGHTGRAY;
  staticModel.materials[0].shader = shadowShader;

  // Broken Pillar Shards
  auto brokenData = assetLoader.getAssetData("pillar_broken.glb");
  m_brokenPillarModel = assetLoader.LoadModelFromAssetData(brokenData, ".glb");
  for (int i = 0; i < m_brokenPillarModel.materialCount; i++) {
    m_brokenPillarModel.materials[i].shader = shadowShader;
  }

  int numPillars = 6;
  float hexRadius = 60.0f;
  float pillarCenterY = 0.0f;

  for (int i = 0; i < numPillars; i++) {
    auto pillar = std::make_shared<GameObject>();
    pillar->name = "PILLAR_" + std::to_string(i);
    pillar->isDestructible = true;
    pillar->health = 3;
    float angle = (i * 60.0f) * DEG2RAD;
    float posX = cos(angle) * hexRadius;
    float posZ = sin(angle) * hexRadius;

    pillar->position = {posX, pillarCenterY, posZ};
    pillar->scale = {1.5f, 2.0f, 1.5f};
    pillar->AddComponent<DrawableComponent>(staticModel);

    btCollisionShape *pillarShape =
        new btBoxShape(btVector3(0.75f, 17.5f, 0.75f));
    pillar->AddComponent<PhysicsComponent>(
        &m_engine->GetPhysicsSystem(), pillarShape, PhysicsShapeType::BOX, 0.0f,
        pillar->position, QuaternionIdentity());

    gameObjectManager.AddGameObject(pillar);
  }

  // ARENA Setup
  auto arenaData = assetLoader.getAssetData("arena.glb");
  Model arenaModel = assetLoader.LoadModelFromAssetData(arenaData, ".glb");
  Texture2D arenaTex =
      arenaModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture;
  SetTextureFilter(arenaTex, TEXTURE_FILTER_POINT);
  arenaModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
  for (int i = 0; i < arenaModel.materialCount; i++) {
    arenaModel.materials[i].shader = shadowShader;
  }

  auto arenaObj = std::make_shared<GameObject>();
  arenaObj->name = "Hexa Arena";
  arenaObj->position = {0.0f, 0.0f, 0.0f};
  arenaObj->scale = {5.0f, 2.5f, 5.0f};
  arenaObj->AddComponent<DrawableComponent>(arenaModel);

  Mesh arenaMesh = arenaModel.meshes[0];
  int numIndices = arenaMesh.triangleCount * 3;
  m_convertedIndices = new int[numIndices];
  for (int i = 0; i < numIndices; ++i) {
    m_convertedIndices[i] = (int)arenaMesh.indices[i];
  }

  btScalar *vertexBase = (btScalar *)arenaMesh.vertices;
  m_indexVertexArray = new btTriangleIndexVertexArray(
      arenaMesh.triangleCount, m_convertedIndices, sizeof(int) * 3,
      arenaMesh.vertexCount, vertexBase, sizeof(float) * 3);

  btBvhTriangleMeshShape *arenaShape =
      new btBvhTriangleMeshShape(m_indexVertexArray, true);
  arenaShape->setLocalScaling(btVector3(5.0f, 2.5f, 5.0f));
  arenaObj->AddComponent<PhysicsComponent>(
      &m_engine->GetPhysicsSystem(), arenaShape, PhysicsShapeType::MESH, 0.0f,
      arenaObj->position, QuaternionIdentity());
  gameObjectManager.AddGameObject(arenaObj);

  m_engine->GetPhysicsSystem().setG(25.0f);

  m_roofLight = std::make_shared<GameObject>();
  m_roofLight->name = "Roof Light";
  m_roofLight->position = m_bulbPosition;
  Model lightModel = LoadModelFromMesh(GenMeshSphere(3.0f, 16, 16));
  lightModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = m_levelColor;
  m_roofLight->AddComponent<DrawableComponent>(lightModel);
  gameObjectManager.AddGameObject(m_roofLight);

  m_arsenal = std::make_shared<Weapon>(m_engine);
  m_arsenal->Initialize();

  Camera &camera = m_engine->GetCamera();
  camera.position = {0.0f, 2.0f, 10.0f};
  camera.target = {0.0f, 0.0f, 0.0f};
  camera.up = {0.0f, 1.0f, 0.0f};
  camera.fovy = 60.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  auto player = std::make_shared<GameObject>();
  player->name = "PLAYER";
  player->position = {0.0f, 5.0f, 0.0f};

  btCollisionShape *pShape = new btCapsuleShape(0.5f, 1.0f);
  auto phys = player->AddComponent<PhysicsComponent>(
      &m_engine->GetPhysicsSystem(), pShape, PhysicsShapeType::CAPSULE, 70.0f,
      player->position, QuaternionIdentity());

  m_playerBody = phys->GetRigidBody();
  m_playerBody->setAngularFactor(btVector3(0.0f, 0.0f, 0.0f));
  m_playerBody->setActivationState(DISABLE_DEACTIVATION);

  gameObjectManager.AddGameObject(player);

  // CUBE Setup
  auto testCube = std::make_shared<GameObject>();
  testCube->name = "Red Test Cube";
  testCube->position = {5.0f, 1.0f, 5.0f};
  Model cubeModel = LoadModelFromMesh(GenMeshCube(2.0f, 2.0f, 2.0f));
  cubeModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = RED;
  cubeModel.materials[0].shader = shadowShader;
  testCube->AddComponent<DrawableComponent>(cubeModel);

  btCollisionShape *cubeShape = new btBoxShape(btVector3(1.0f, 1.0f, 1.0f));
  testCube->AddComponent<PhysicsComponent>(
      &m_engine->GetPhysicsSystem(), cubeShape, PhysicsShapeType::BOX, 10.0f,
      testCube->position, QuaternionIdentity());
  gameObjectManager.AddGameObject(testCube);

  // LEVEL LOADER
  auto savedData = m_levelEditor.LoadLevelData("levels/level001_aru.wushang");
  if (!savedData.empty()) {
    auto objects = gameObjectManager.GetGameObjects();
    for (size_t i = 0; i < savedData.size() && i < objects.size(); i++) {
      objects[i]->position = savedData[i].position;
      objects[i]->scale = savedData[i].scale;
      objects[i]->rotation =
          QuaternionFromEuler(savedData[i].rotation.x, savedData[i].rotation.y,
                              savedData[i].rotation.z);

      auto physicsObj = objects[i]->GetComponent<PhysicsComponent>();
      if (physicsObj && physicsObj->GetRigidBody()) {
        btTransform trans;
        physicsObj->GetRigidBody()->getMotionState()->getWorldTransform(trans);
        trans.setOrigin(btVector3(savedData[i].position.x,
                                  savedData[i].position.y,
                                  savedData[i].position.z));
        physicsObj->GetRigidBody()->setWorldTransform(trans);
        physicsObj->GetRigidBody()->getMotionState()->setWorldTransform(trans);
      }
    }
  }

  DisableCursor();
}

void WuShangClanScene::Update(float deltaTime) {
  auto &physicsSystem = m_engine->GetPhysicsSystem();
  auto &gameObjectManager = m_engine->GetGameObjectManager();
  auto &camera = m_engine->GetCamera();

  m_chatSystem.Update(deltaTime);

  if (IsKeyPressed(KEY_TAB)) {
    m_levelEditor.ToggleEditor();
    if (m_levelEditor.IsActive()) {
      EnableCursor();
      m_levelEditor.SyncCamera(m_cameraYaw, m_cameraPitch);
    } else
      DisableCursor();
  }

  if (IsKeyPressed(KEY_O)) {
    m_logicEditor.ToggleEditor();
  }

  if (m_logicEditor.IsActive()) {
    if (IsCursorHidden())
      EnableCursor(); // Editor on -> Mouse Azaad
  } else {
    if (!IsCursorHidden())
      DisableCursor(); // Editor off -> Mouse Lock
  }

  if (IsKeyPressed(KEY_P))
    m_engine->PostProcessingActive = !m_engine->PostProcessingActive;

  if (m_levelEditor.IsActive()) {
    auto objects = gameObjectManager.GetGameObjects();
    m_levelEditor.Update(camera, objects);
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S))
      m_levelEditor.SaveLevel("levels/level001_aru.wushang", objects);
  } else {
    physicsSystem.Update(deltaTime);
    gameObjectManager.Update(deltaTime);

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && m_arsenal->currentWeapon == 0 &&
        m_arsenal->spamMode == 2 && m_arsenal->m_fireRateTimer <= 0.0f) {
      m_chatSystem.OnCancelledUsed();
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) &&
        m_arsenal->m_fireRateTimer <= 0.0f) {
      Vector3 forward = {cosf(m_cameraPitch) * sinf(m_cameraYaw),
                         sinf(m_cameraPitch),
                         cosf(m_cameraPitch) * cosf(m_cameraYaw)};
      Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
      Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));

      Vector3 muzzlePos = camera.position;
      muzzlePos = Vector3Add(muzzlePos, Vector3Scale(forward, 1.5f));
      muzzlePos = Vector3Add(muzzlePos, Vector3Scale(right, 0.5f));
      muzzlePos = Vector3Subtract(muzzlePos, Vector3Scale(up, 0.2f));

      if (m_arsenal->currentWeapon == 0)
        emitterSmoke->Emit(2, muzzlePos);
      else if (m_arsenal->currentWeapon == 1) {
        Vector3 blastPos = camera.position;
        blastPos.y -= 1.0f;
        emitterShockwave->Emit(1, blastPos);
      }
    }

    m_arsenal->Update(GetFrameTime());

    // Vector3 smokeOrigin ;
    Vector3 rainOrigin = {camera.position.x, camera.position.y + 20.0f,
                          camera.position.z};

    // emitterSmoke->Update(deltaTime, smokeOrigin);
    emitterSparks->Update(deltaTime);
    emitterRain->Update(deltaTime, rainOrigin);
    emitterSplash->Update(deltaTime);
    emitterShockwave->Update(deltaTime);

    Vector2 mouseDelta = GetMouseDelta();
    m_cameraYaw -= mouseDelta.x * 0.002f;
    m_cameraPitch -= mouseDelta.y * 0.002f;

    if (m_cameraPitch > 1.5f)
      m_cameraPitch = 1.5f;
    if (m_cameraPitch < -1.5f)
      m_cameraPitch = -1.5f;

    Vector3 forward = {cosf(m_cameraPitch) * sinf(m_cameraYaw),
                       sinf(m_cameraPitch),
                       cosf(m_cameraPitch) * cosf(m_cameraYaw)};
    Vector3 walkDir = Vector3Normalize({forward.x, 0.0f, forward.z});
    Vector3 rightDir = {-walkDir.z, 0.0f, walkDir.x};

    m_isSliding = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_LEFT_SHIFT);
    m_playerBody->setFriction(m_isSliding ? 0.0f : 0.8f);

    Vector3 wishDir = {0, 0, 0};
    if (IsKeyDown(KEY_W))
      wishDir = Vector3Add(wishDir, walkDir);
    if (IsKeyDown(KEY_S))
      wishDir = Vector3Subtract(wishDir, walkDir);
    if (IsKeyDown(KEY_D))
      wishDir = Vector3Add(wishDir, rightDir);
    if (IsKeyDown(KEY_A))
      wishDir = Vector3Subtract(wishDir, rightDir);
    if (Vector3Length(wishDir) > 0.0f)
      wishDir = Vector3Normalize(wishDir);

    float maxSpeed = m_isSliding ? 60.0f : 30.0f;
    btVector3 currentVel = m_playerBody->getLinearVelocity();
    float targetX = wishDir.x * maxSpeed;
    float targetZ = wishDir.z * maxSpeed;
    float accel = m_isSliding ? 0.5f : 8.0f;

    if (abs(currentVel.x()) < maxSpeed || (currentVel.x() * targetX < 0))
      m_playerBody->applyCentralImpulse(
          btVector3((targetX - currentVel.x()) * accel, 0, 0));
    if (abs(currentVel.z()) < maxSpeed || (currentVel.z() * targetZ < 0))
      m_playerBody->applyCentralImpulse(
          btVector3(0, 0, (targetZ - currentVel.z()) * accel));

    if (IsKeyPressed(KEY_SPACE) && abs(currentVel.y()) < 0.1f) {
      m_playerBody->setLinearVelocity(
          btVector3(currentVel.x(), 20.0f, currentVel.z()));
    }

    btTransform trans;
    m_playerBody->getMotionState()->getWorldTransform(trans);
    float eyeHeight = m_isSliding ? -0.2f : 0.6f;
    camera.position = {(float)trans.getOrigin().x(),
                       (float)trans.getOrigin().y() + eyeHeight,
                       (float)trans.getOrigin().z()};
    camera.target = Vector3Add(camera.position, forward);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      m_shakeTimer = (m_arsenal->currentWeapon == 0) ? 0.15f : 0.6f;
      m_shakeIntensity = (m_arsenal->currentWeapon == 0) ? 0.1f : 0.8f;
    }

    if (m_shakeTimer > 0.0f) {
      m_shakeTimer -= deltaTime;
      float currentIntensity = m_shakeIntensity * (m_shakeTimer + 0.1f);
      float offsetX = (GetRandomValue(-100, 100) / 100.0f) * currentIntensity;
      float offsetY = (GetRandomValue(-100, 100) / 100.0f) * currentIntensity;
      float offsetZ = (GetRandomValue(-100, 100) / 100.0f) * currentIntensity;
      camera.position.x += offsetX;
      camera.position.y += offsetY;
      camera.position.z += offsetZ;
      camera.target.x += offsetX;
      camera.target.y += offsetY;
      camera.target.z += offsetZ;
    }

    auto objects = gameObjectManager.GetGameObjects();
    std::vector<std::shared_ptr<GameObject>> objectsToDestroy,
        projectilesToDestroy, objectsToShatter;

    for (const auto &obj : objects) {
      if (obj->timeToLive > 0.0f) {
        obj->timeToLive -= deltaTime;
        if (obj->timeToLive <= 0.0f) {
          objectsToDestroy.push_back(obj);
          continue;
        }
      }

      if (obj->name.find("PROJECTILE|") != std::string::npos) {
        auto physComp = obj->GetComponent<PhysicsComponent>();
        if (!physComp)
          continue;

        btRigidBody *rb = physComp->GetRigidBody();
        if (rb)
          rb->setGravity(btVector3(0, 0, 0)); // Engine override proof

        if (Vector3Distance(camera.position, obj->position) > 5000.0f) {
          projectilesToDestroy.push_back(obj);
          continue;
        }

        // ==============================================
        // ULTIMATE PHYSICS IMPACT DETECTION
        // Jab bhi goli kisi bhi cheez (arena wall, floor, pillar, cube) se
        // physically takrayegi, uski velocity automatically drop ho jayegi!
        // ==============================================
        if (rb) {
          float speed = rb->getLinearVelocity().length();

          // Allow 0.02s (1.98f) safety window taaki bullet galti se barrel me
          // spawn hote hi hit detect na kare Agar bullet tezi (60+ speed) se
          // chal rahi thi aur achanak uski speed < 45 hui, matlab wo FAT GAYI!
          if (obj->timeToLive < 1.98f && speed < 45.0f) {

            // SPARK EXACTLY AT THE HIT SURFACE!
            emitterSparks->Emit(30, obj->position);

            // AREA OF EFFECT DAMAGE (radius 2.5f)
            for (auto &target : objects) {
              if (target == obj || target->name == "PLAYER" ||
                  target->name.find("PROJECTILE") != std::string::npos)
                continue;

              bool inRange = false;

              if (target->name.find("PILLAR") != std::string::npos) {
                float dx = obj->position.x - target->position.x;
                float dz = obj->position.z - target->position.z;
                float distXZ = sqrtf(dx * dx + dz * dz);

                // Check: XZ distance 3.0 ke andar ho, aur goli pillar ki height
                // limit ke beech ho
                if (distXZ < 3.0f && obj->position.y >= -2.0f &&
                    obj->position.y <= 35.0f) {
                  inRange = true;
                }
              } else {
                // Normal Sphere distance check (Red Cube waghera ke liye)
                if (Vector3Distance(obj->position, target->position) < 3.0f) {
                  inRange = true;
                }
              }

              // Agar target limit mein hai, toh Damage de do!
              if (inRange) {
                if (target->isDestructible) {
                  target->health -= 1;
                  if (target->health <= 0)
                    objectsToShatter.push_back(target);
                }
              }
            }

            if (obj->name.find("CRINGE") != std::string::npos)
              m_chatSystem.OnPerfectInterrupt();
            else
              m_chatSystem.OnComboSuccess();

            projectilesToDestroy.push_back(obj);
            continue;
          }
        }
      }

      if (auto enemy = std::dynamic_pointer_cast<Enemy>(obj)) {
        enemy->UpdateAI(camera.position, deltaTime);
        static float damageCooldown = 0.0f;
        if (damageCooldown > 0.0f)
          damageCooldown -= deltaTime;

        if (Vector3Distance(camera.position, enemy->position) < 3.0f &&
            damageCooldown <= 0.0f) {
          m_chatSystem.OnPlayerTakeDamage();
          damageCooldown = 2.0f;
          emitterSplash->Emit(50, camera.position);
          Vector3 pushBack = Vector3Normalize(
              Vector3Subtract(camera.position, enemy->position));
          m_playerBody->applyCentralImpulse(
              btVector3(pushBack.x * 50.0f, 20.0f, pushBack.z * 50.0f));
          m_shakeTimer = 0.4f;
          m_shakeIntensity = 0.5f;
        }
      }
    }

    if (IsKeyPressed(KEY_L))
      gameObjectManager.AddGameObject(std::make_shared<Enemy>(
          m_engine, BossType::SUNRAYBEE,
          Vector3Add(camera.position, Vector3Scale(forward, 15.0f))));

    for (auto &obj : objectsToDestroy)
      gameObjectManager.RemoveGameObject(obj);
    for (auto &proj : projectilesToDestroy)
      gameObjectManager.RemoveGameObject(proj);

    for (auto &brokenPillar : objectsToShatter) {
      Vector3 deathPos = brokenPillar->position;
      gameObjectManager.RemoveGameObject(brokenPillar);

      emitterSmoke->Emit(50, deathPos);
      emitterSparks->Emit(20, deathPos);
      m_shakeTimer = 0.8f;
      m_shakeIntensity = 1.2f;

      for (int i = 0; i < m_brokenPillarModel.meshCount; i++) {
        auto shard = std::make_shared<GameObject>();
        shard->name = "SHARD_" + std::to_string(i);
        shard->position = deathPos;
        Model shardModel = m_brokenPillarModel;
        shardModel.meshCount = 1;
        shardModel.meshes = &m_brokenPillarModel.meshes[i];
        shardModel.meshMaterial = &m_brokenPillarModel.meshMaterial[i];
        shard->AddComponent<DrawableComponent>(shardModel);

        auto phys = shard->AddComponent<PhysicsComponent>(
            &physicsSystem, new btBoxShape(btVector3(0.4f, 0.4f, 0.4f)),
            PhysicsShapeType::BOX, 10.0f, shard->position,
            QuaternionIdentity());
        phys->GetRigidBody()->activate(true);
        phys->GetRigidBody()->setLinearVelocity(
            btVector3(GetRandomValue(-15, 15), GetRandomValue(10, 20),
                      GetRandomValue(-15, 15)));
        shard->timeToLive = GetRandomValue(30, 50) / 10.0f;
        gameObjectManager.AddGameObject(shard);
      }
    }
  }

  Shader shadowShader = m_engine->GetRenderer().GetShadowShader();
  if (int loc = GetShaderLocation(shadowShader, "lightPos"); loc != -1)
    SetShaderValue(shadowShader, loc, &camera.position, SHADER_UNIFORM_VEC3);
  if (int loc = GetShaderLocation(shadowShader, "lightColor"); loc != -1) {
    float c[4] = {m_levelColor.r / 255.0f, m_levelColor.g / 255.0f,
                  m_levelColor.b / 255.0f, 1.0f};
    SetShaderValue(shadowShader, loc, c, SHADER_UNIFORM_VEC4);
  }
  if (int loc = GetShaderLocation(shadowShader, "lightDir"); loc != -1) {
    Vector3 straightDown = {0.001f, -1.0f, 0.001f};
    SetShaderValue(shadowShader, loc, &straightDown, SHADER_UNIFORM_VEC3);
  }

  auto &postProcessing = m_engine->GetPostProcessing();
  postProcessing.SetShader(FX_BRAINROT);
  float time = (float)GetTime();
  SetShaderValue(postProcessing.GetCurrentShader(), postProcessing.timeLoc,
                 &time, SHADER_UNIFORM_FLOAT);
}

void WuShangClanScene::Draw3D() {
  auto objects = m_engine->GetGameObjectManager().GetGameObjects();
  for (const auto &obj : objects) {
    if (auto enemy = std::dynamic_pointer_cast<Enemy>(obj)) {
      if (enemy->m_isSprite) {
        Rectangle sourceRec = {0.0f, 0.0f, (float)enemy->m_spriteTexture.width,
                               (float)enemy->m_spriteTexture.height};
        DrawBillboardRec(m_engine->GetCamera(), enemy->m_spriteTexture,
                         sourceRec, enemy->position, enemy->m_spriteSize,
                         WHITE);
      }
    }
  }

  globalVFX.Draw(m_engine->GetCamera());
}

void WuShangClanScene::Draw() {
  m_arsenal->DrawHUD();
  m_chatSystem.DrawHUD();

  rlImGuiBegin();

  m_levelEditor.Draw();
  m_logicEditor.UpdateAndDraw();

  rlImGuiEnd();
}

void WuShangClanScene::Shutdown() {
  EnableCursor();

  if (m_convertedIndices)
    delete[] m_convertedIndices;
  if (m_indexVertexArray)
    delete m_indexVertexArray;
}
