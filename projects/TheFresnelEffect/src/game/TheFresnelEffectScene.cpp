#include "game/TheFresnelEffectScene.h"
#include "engine/DrawableComponent.h"
#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/PhysicsComponent.h"
#include "game/Water.h"
#include "raylib.h"
#include "raymath.h"

TheFresnelEffectScene::TheFresnelEffectScene() : m_engine(nullptr) {}

TheFresnelEffectScene::~TheFresnelEffectScene() {}

void TheFresnelEffectScene::Initialize(Engine *engine) {
  Scene3D::Initialize(engine);
  m_engine = engine;
  auto &gameObjectManager = m_engine->GetGameObjectManager();

  Shader shadowShader = m_engine->GetRenderer().GetShadowShader();

  int ambientLoc = GetShaderLocation(shadowShader, "ambient");
  if (ambientLoc != -1) {
    float ambientLight[4] = {0.1f, 0.1f, 0.1f, 1.0f}; // Adjust for brightness
    SetShaderValue(shadowShader, ambientLoc, ambientLight, SHADER_UNIFORM_VEC4);
  }

  // auto cube = std::make_shared<GameObject>();
  // cube->name = "Sample Cube";
  // cube->position = {0, 2, 0};
  //
  // Model cubeModel = LoadModelFromMesh(GenMeshCube(2.0f, 2.0f, 2.0f));
  // cubeModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = RED;
  //
  // cubeModel.materials[0].shader = shadowShader;
  //
  // cube->AddComponent<DrawableComponent>(cubeModel);
  //
  // btCollisionShape *shape = new btBoxShape(btVector3(1.0f, 1.0f, 1.0f));
  // cube->AddComponent<PhysicsComponent>(&m_engine->GetPhysicsSystem(), shape,
  //                                      PhysicsShapeType::BOX,
  //                                      0.0f, // Mass 0 = Static
  //                                      cube->position, QuaternionIdentity());
  // gameObjectManager.AddGameObject(cube);

  m_water = std::make_shared<Water>(m_engine, Vector3{0, 0, 0},
                                    Vector2{500.0f, 500.0f});

  auto &assetLoader = m_engine->GetAssetLoader();

  assetLoader.loadPackedFile("shaders.bin");
  assetLoader.loadPackedFile("map.pack");

  m_testmap = assetLoader.LoadModelFromAssetData(
      assetLoader.getAssetData("lake.glb"), ".glb");

  for (int i = 0; i < m_testmap.materialCount; i++) {
    m_testmap.materials[i].shader = shadowShader;

    if (m_testmap.materials[i].maps[MATERIAL_MAP_ALBEDO].texture.id == 0) {
      Image dummyImg = GenImageColor(1, 1, WHITE);
      m_testmap.materials[i].maps[MATERIAL_MAP_ALBEDO].texture =
          LoadTextureFromImage(dummyImg);
      UnloadImage(dummyImg);
    }
  }

  // map object registry
  auto mapObject = std::make_shared<GameObject>();
  mapObject->position = Vector3{0, -10, 0};
  mapObject->scale = Vector3{10.0f, 10.0f, 10.0f};
  mapObject->AddComponent<DrawableComponent>(m_testmap);
  mapObject->name = "map";
  gameObjectManager.AddGameObject(mapObject);

  auto player = std::make_shared<GameObject>();
  player->name = "PLAYER_CAM";
  player->position = {0.0f, 2.0f, 10.0f};

  btCollisionShape *pShape = new btCapsuleShape(0.5f, 1.0f);
  auto phys = player->AddComponent<PhysicsComponent>(
      &m_engine->GetPhysicsSystem(), pShape, PhysicsShapeType::CAPSULE, 70.0f,
      player->position, QuaternionIdentity());

  m_playerBody = phys->GetRigidBody();
  m_playerBody->setAngularFactor(
      btVector3(0.0f, 0.0f, 0.0f));                       // No tipping over
  m_playerBody->setActivationState(DISABLE_DEACTIVATION); // Never sleep

  // Disable gravity for the camera so you can easily fly around!
  m_playerBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));

  gameObjectManager.AddGameObject(player);

  // Setup Camera
  Camera &camera = m_engine->GetCamera();
  camera.position = {0.0f, 2.0f, 10.0f};
  camera.target = {0.0f, 0.0f, 0.0f};
  camera.up = {0.0f, 1.0f, 0.0f};
  camera.fovy = 60.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  DisableCursor();
}

void TheFresnelEffectScene::Update(float deltaTime) {
  auto &camera = m_engine->GetCamera();

  auto &physicsSystem = m_engine->GetPhysicsSystem();
  auto &gameObjectManager = m_engine->GetGameObjectManager();

  if (IsKeyPressed(KEY_ESCAPE)) {
    if (IsCursorHidden())
      EnableCursor();
    else
      DisableCursor();
  }

  if (m_water) {
    m_water->Update(deltaTime);
  }

  Shader shadowShader = m_engine->GetRenderer().GetShadowShader();
  if (int loc = GetShaderLocation(shadowShader, "lightPos"); loc != -1) {
    SetShaderValue(shadowShader, loc, &camera.position, SHADER_UNIFORM_VEC3);
  }
  if (int loc = GetShaderLocation(shadowShader, "lightDir"); loc != -1) {
    // Slant light vectors for high depth shadows on mountain valleys
    Vector3 terrainLight = Vector3Normalize({0.4f, -0.6f, -0.4f});
    SetShaderValue(shadowShader, loc, &terrainLight, SHADER_UNIFORM_VEC3);
  }

  if (int loc = GetShaderLocation(shadowShader, "lightColor"); loc != -1) {
    float lightColor[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // White sunlight
    SetShaderValue(shadowShader, loc, lightColor, SHADER_UNIFORM_VEC4);
  }

  if (IsCursorHidden()) {
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

    Vector3 wishDir = {0, 0, 0};
    if (IsKeyDown(KEY_W))
      wishDir = Vector3Add(wishDir, walkDir);
    if (IsKeyDown(KEY_S))
      wishDir = Vector3Subtract(wishDir, walkDir);
    if (IsKeyDown(KEY_D))
      wishDir = Vector3Add(wishDir, rightDir);
    if (IsKeyDown(KEY_A))
      wishDir = Vector3Subtract(wishDir, rightDir);
    if (IsKeyDown(KEY_SPACE))
      wishDir.y += 1.0f;
    if (IsKeyDown(KEY_LEFT_SHIFT))
      wishDir.y -= 1.0f;

    if (Vector3Length(wishDir) > 0.0f)
      wishDir = Vector3Normalize(wishDir);

    float speed = 15.0f;
    m_playerBody->setLinearVelocity(
        btVector3(wishDir.x * speed, wishDir.y * speed, wishDir.z * speed));

    physicsSystem.Update(deltaTime);
    gameObjectManager.Update(deltaTime);

    btTransform trans;
    m_playerBody->getMotionState()->getWorldTransform(trans);

    camera.position = {(float)trans.getOrigin().x(),
                       (float)trans.getOrigin().y() + 0.6f,
                       (float)trans.getOrigin().z()};
    camera.target = Vector3Add(camera.position, forward);
  } else {
    m_playerBody->setLinearVelocity(btVector3(0, 0, 0));
  }
}
void TheFresnelEffectScene::Draw() {}

void TheFresnelEffectScene::Draw3D() {
  if (m_water) {
    auto drawable = m_water->GetComponent<DrawableComponent>();
    if (drawable) {
      // CRITICAL FIX: Update your texture maps immediately before the draw
      // calls execute. This ensures your shader samples the fresh depth maps
      // drawn during this frame pass!
      m_water->GetModel().materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture =
          m_engine->GetRenderer().GetTarget().depth;
      m_water->GetModel().materials[0].maps[MATERIAL_MAP_EMISSION].texture =
          m_engine->GetRenderer().GetReflectionTarget().texture;

      drawable->Draw();
    }
  }
}

void TheFresnelEffectScene::Shutdown() { EnableCursor(); }
