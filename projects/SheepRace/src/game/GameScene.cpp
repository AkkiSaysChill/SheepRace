#include "game/GameScene.h"
#include "engine/AnimationComponent.h"
#include "engine/DrawableComponent.h"
#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/PhysicsComponent.h"
#include "engine/SceneManager.h"
#include "engine/editorUi.h"
#include "engine/particleSystem.h"
#include "imgui.h"
#include "raylib.h"
#include "raymath.h"
#include "rcamera.h"
#include "rlImGui.h"
#include "rlgl.h"
#include <btBulletDynamicsCommon.h>
#include <iostream>
#include <memory>

GameScene::GameScene()
    : m_engine(nullptr), m_playerSheep(nullptr), m_tool_active(true),
      m_fov(45.0f), m_shininess(32.0f), m_specularStrength(0.5f),
      m_lightDir({0.35f, -1.0f, -0.35f}), m_postProcessingActive(false),
      m_cloudCover(0.4f), m_cloudFalloff(0.8f), m_lightCamOrthoSize(200.0f),
      m_cameraMouseMoveSensitivity(0.01f) {}

GameScene::~GameScene() {}

void PrintModelDetails(Model model, const char *modelName) {
  std::cout << "\n--- DETAILS FOR: " << modelName << " ---" << std::endl;
  std::cout << "Mesh Count: " << model.meshCount << std::endl;
  std::cout << "Material Count: " << model.materialCount << std::endl;
  std::cout << "Bone Count: " << model.boneCount << std::endl;

  for (int i = 0; i < model.meshCount; i++) {
    Mesh m = model.meshes[i];
    std::cout << "  [Mesh " << i << "]" << std::endl;
    std::cout << "    Vertices: " << m.vertexCount << std::endl;
    std::cout << "    Has Normals: " << (m.normals != nullptr ? "YES" : "NO")
              << std::endl;
    std::cout << "    Has TexCoords: "
              << (m.texcoords != nullptr ? "YES" : "NO") << std::endl;
    std::cout << "    Has Tangents: " << (m.tangents != nullptr ? "YES" : "NO")
              << std::endl;
    std::cout << "    Has Bone IDs: " << (m.boneIds != nullptr ? "YES" : "NO")
              << std::endl;
    std::cout << "    Has Bone Weights: "
              << (m.boneWeights != nullptr ? "YES" : "NO") << std::endl;
  }
  std::cout << "-----------------------------------\n" << std::endl;
}

void GameScene::Initialize(Engine *engine) {
  Scene3D::Initialize(engine);
  m_engine = engine;

  // Get references to engine systems
  auto &physicsSystem = m_engine->GetPhysicsSystem();
  auto &gameObjectManager = m_engine->GetGameObjectManager();
  auto &renderer = m_engine->GetRenderer();
  auto &assetLoader = m_engine->GetAssetLoader();

  // load save
  m_progress.Load("player.save");

  std::cout << "Current Currency: " << m_progress.totalCurrency << std::endl;

  // Load scene-specific assets
  assetLoader.loadPackedFile("levels.map");
  m_testmap = assetLoader.LoadModelFromAssetData(
      assetLoader.getAssetData("testMap.glb"), "testMap.glb");

  auto smokeData = assetLoader.getAssetData("smoke.png");
  Texture2D smoke_tex = assetLoader.LoadTextureFromAssetData(smokeData, ".png");

  // Setup scene
  Mesh mapMesh = m_testmap.meshes[0];
  int numIndices = mapMesh.triangleCount * 3;
  m_convertedIndices = new int[numIndices];
  for (int i = 0; i < numIndices; ++i) {
    m_convertedIndices[i] = (int)mapMesh.indices[i];
  }
  btScalar *vertexBase = (btScalar *)mapMesh.vertices;
  m_indexVertexArray = new btTriangleIndexVertexArray(
      mapMesh.triangleCount, m_convertedIndices, sizeof(int) * 3,
      mapMesh.vertexCount, vertexBase, sizeof(float) * 3);
  btBvhTriangleMeshShape *mapShape =
      new btBvhTriangleMeshShape(m_indexVertexArray, true);

  // map object
  auto mapObject = std::make_shared<GameObject>();
  mapShape->setLocalScaling(btVector3(4.0f, 4.0f, 4.0f));
  mapObject->AddComponent<PhysicsComponent>(
      &physicsSystem, mapShape, PhysicsShapeType::MESH, 0.0f, Vector3{0, 0, 0},
      Quaternion{0, 0, 0, 1});
  mapObject->position = Vector3{0, 0, 0};
  mapObject->scale = Vector3{4.0f, 4.0f, 4.0f};
  mapObject->AddComponent<DrawableComponent>(m_testmap);
  mapObject->name = "map";
  gameObjectManager.AddGameObject(mapObject);

  // test object for the game hierarchy
  Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
  Model crateModel = LoadModelFromMesh(cubeMesh);

  // --- Create the GameObject ---
  auto crate = std::make_shared<GameObject>();
  crate->name = "Wooden Crate";                 // Helpful for your EditorUI
  crate->position = Vector3{5.0f, 10.0f, 0.0f}; // Drop it from height
  crate->scale = Vector3{1.0f, 1.0f, 1.0f};
  crate->AddComponent<DrawableComponent>(crateModel);
  gameObjectManager.AddGameObject(crate);

  btCollisionShape *fallShape = new btSphereShape(1);
  auto fallingSphere = std::make_shared<GameObject>();
  fallingSphere->AddComponent<PhysicsComponent>(
      &physicsSystem, fallShape, PhysicsShapeType::SPHERE, 1.0f,
      Vector3{5, 40, 0}, Quaternion{0, 0, 0, 1});
  fallingSphere->name = "ball";
  Mesh sphereMesh = GenMeshSphere(1.0f, 16, 16);
  m_sphereModel = LoadModelFromMesh(sphereMesh);
  fallingSphere->AddComponent<DrawableComponent>(m_sphereModel);
  gameObjectManager.AddGameObject(fallingSphere);

  // Create player
  auto playerShared = std::make_shared<Car>(&physicsSystem, assetLoader);
  playerShared->name = "player";
  m_playerSheep = playerShared.get();
  gameObjectManager.AddGameObject(playerShared);

  // The engine creates the renderer now, so we don't do it here.

  // The camera is now owned by the engine. We just set its initial position.
  auto &camera = m_engine->GetCamera();
  Vector3 cameraOffset = {0.0f, 5.0f, -10.0f};
  camera.position = Vector3Add(m_playerSheep->GetPosition(), cameraOffset);

  // Assign shaders
  Shader shadowShader = renderer.GetShadowShader();
  for (int i = 0; i < m_testmap.materialCount; i++) {
    m_testmap.materials[i].shader = shadowShader;
  }
  m_sphereModel.materials[0].shader = shadowShader;
  m_playerSheep->AssignShader(shadowShader);

  auto charData = assetLoader.getAssetData("runner.glb");
  Model charModel = assetLoader.LoadModelFromAssetData(charData, "runner.glb");
  int animCount = 0;
  ModelAnimation *anims =
      assetLoader.LoadAnimationsFromAssetData(charData, &animCount);

  auto runner = std::make_shared<GameObject>();
  runner->name = "Runner";
  runner->position = {5, 1, 5};
  runner->scale = {0.5f, 0.5f, 0.5f};
  auto drawable = runner->AddComponent<DrawableComponent>(charModel, true);
  gameObjectManager.AddGameObject(runner);

  // m_smoke = std::make_shared<ParticleSystem>(100, smoke_tex);

  PrintModelDetails(playerShared->model, "Sheep (Working)");
  PrintModelDetails(charModel, "runner (broken)");

  auto savedData = m_levelEditor.LoadLevelData("level001");

  if (!savedData.empty()) {
    auto objects = gameObjectManager.GetGameObjects();

    // Har object ko uski saved position par bhej do
    for (size_t i = 0; i < savedData.size() && i < objects.size(); i++) {
      objects[i]->position = savedData[i].position;
      objects[i]->scale = savedData[i].scale;
      objects[i]->rotation =
          QuaternionFromEuler(savedData[i].rotation.x, savedData[i].rotation.y,
                              savedData[i].rotation.z);

      // PHYSICS UPDATE (Bohot zaroori, warna invisible deewar se takrayega
      // player)
      auto phys = objects[i]->GetComponent<PhysicsComponent>();
      if (phys && phys->GetRigidBody()) {
        btTransform trans;
        phys->GetRigidBody()->getMotionState()->getWorldTransform(trans);
        trans.setOrigin(btVector3(savedData[i].position.x,
                                  savedData[i].position.y,
                                  savedData[i].position.z));
        phys->GetRigidBody()->setWorldTransform(trans);
        phys->GetRigidBody()->getMotionState()->setWorldTransform(trans);
      }
    }
  }
}

void GameScene::Update(float deltaTime) {
  auto &physicsSystem = m_engine->GetPhysicsSystem();
  auto &gameObjectManager = m_engine->GetGameObjectManager();
  auto &camera = m_engine->GetCamera();

  physicsSystem.Update(deltaTime);
  m_smoke->Update(GetFrameTime());

  if (IsKeyPressed(KEY_TAB)) {
    m_levelEditor.ToggleEditor();

    if (m_levelEditor.IsActive()) {
      EnableCursor(); // Editor mode mein mouse laao
      // Jadu: Editor camera ko batao player kahan dekh raha tha!
      m_levelEditor.SyncCamera(m_cameraYaw, m_cameraPitch);
      std::cout << "[EDITOR] Mode: ON. Game Paused." << std::endl;
    } else {
      DisableCursor(); // Wapas FPS mode mein hide karo
      std::cout << "[EDITOR] Mode: OFF. Gameplay resumed." << std::endl;
    }
  }

#ifdef _DEBUG
  if (IsKeyPressed(KEY_I)) {
    m_smoke->Emit({1, 5, 0}, {0, -1, 1}, 1.0f);
    std::cout << "hahaha" << std::endl;
  }
  if (IsKeyPressed(KEY_P)) {
    m_progress.AddCurrency(100);
    m_progress.Save("player.save");
    std::cout << "Added 100 Gold. Saved." << std::endl;
  }
  if (IsKeyPressed(KEY_O)) {
    m_progress.AddXp(100);
    m_progress.Save("player.save");
  }
#endif

  if (m_tempG != m_gravity) {
    m_tempG = m_gravity;
    physicsSystem.setG(m_gravity);
  }
  gameObjectManager.Update(deltaTime);
  m_playerSheep->Update(deltaTime);

  for (auto &bulls : m_bulls) {
    bulls.Update(deltaTime);
  }

  m_lightDir = Vector3Normalize(m_lightDir);

  if (m_levelEditor.IsActive()) {

    // ----------------------------------------
    // MODE A: EDITOR IS ACTIVE (GAME PAUSED)
    // ----------------------------------------
    auto objects = gameObjectManager.GetGameObjects();

    // Editor ab khud apna camera aur mouse sambhalega!
    m_levelEditor.Update(camera, objects);

    // Save Shortcut
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
      m_levelEditor.SaveLevel("level002_jay.wushang", objects);
    }

    else {
      camera.target = m_playerSheep->GetPosition();
      if (!m_tool_active) {
        Vector2 mousePositionDelta = GetMouseDelta();
        CameraYaw(&camera, -mousePositionDelta.x * m_cameraMouseMoveSensitivity,
                  true);
        CameraPitch(&camera,
                    -mousePositionDelta.y * m_cameraMouseMoveSensitivity, true,
                    true, false);
      }
      Vector3 newTargetToRotatedPosition =
          Vector3Subtract(camera.position, camera.target);
      float desiredCameraDistance = 10.0f;
      camera.position =
          Vector3Add(camera.target,
                     Vector3Scale(Vector3Normalize(newTargetToRotatedPosition),
                                  desiredCameraDistance));
      camera.fovy = m_fov;
      UpdateCamera(&camera, CAMERA_CUSTOM);

#ifdef _DEBUG
      if (IsKeyPressed(KEY_M)) {
        m_tool_active = !m_tool_active;
        if (m_tool_active) {
          EnableCursor();
        } else {
          DisableCursor();
        }
      }
#endif
      if (!m_tool_active) {
        DisableCursor();
      }
    }
  }
}

void GameScene::OnResize(int width, int height) {
  // The engine now handles renderer resizing
}

void GameScene::Draw() {
  // The main rendering is now done in the Engine's main loop.
  // This Draw function is for debug overlays and UI.
  auto &camera = m_engine->GetCamera();
  auto &gameObjectManager = m_engine->GetGameObjectManager();
  auto &physicsSystem = m_engine->GetPhysicsSystem();
  auto &renderer = m_engine->GetRenderer();
  auto &assetLoader = m_engine->GetAssetLoader(); // Get the asset loader

#ifdef _DEBUG
  BeginMode3D(camera);
  rlDisableDepthTest();
  for (const auto &obj : gameObjectManager.GetGameObjects()) {
    if (obj->debugPhysics) {
      auto phys = obj->GetComponent<PhysicsComponent>();
      if (phys) {
        phys->DrawDebug();
      }
    }
  }
  if (m_playerSheep)
    m_playerSheep->DrawDebug();
  m_smoke->Draw(camera);
  rlEnableDepthTest();
  EndMode3D();
#endif

  // ImGui
  rlImGuiBegin();
  if (m_tool_active) {
    ImGui::Begin("Game Settings");
    ImGui::SliderFloat("FOV", &m_fov, 0, 300);
    ImGui::Text("Camera FOV: %.2f", camera.fovy);
    ImGui::Text("Camera Position: %.2f, %.2f, %.2f", camera.position.x,
                camera.position.y, camera.position.z);
    ImGui::Text("Camera Target: %.2f, %.2f, %.2f", camera.target.x,
                camera.target.y, camera.target.z);
    ImGui::SliderFloat("gravity", &m_gravity, -30, 30);
    ImGui::Separator();
    ImGui::Text("Car Position: %.2f, %.2f, %.2f",
                m_playerSheep->GetPosition().x, m_playerSheep->GetPosition().y,
                m_playerSheep->GetPosition().z);
    float current_scale = m_playerSheep->scale.x;
    if (ImGui::SliderFloat("Car Scale", &current_scale, 0.1f, 5.0f)) {
      m_playerSheep->setScale(current_scale);
    }
    ImGui::SliderFloat("sensitivity", &m_cameraMouseMoveSensitivity, 0, 1);
    if (ImGui::Button("CloseWindow")) {
      m_tool_active = false;
    }
    ImGui::Separator();
    ImGui::Text("Shadow Light Settings");
    ImGui::SliderFloat("Light Ortho Size", &m_lightCamOrthoSize, 0.0f, 500.0f);
    ImGui::SliderFloat3("Shadow Light Direction", &m_lightDir.x, -1.0f, 1.0f);
    ImGui::Separator();
    ImGui::Text("Material Properties");
    ImGui::SliderFloat("Shininess", &m_shininess, 2.0f, 128.0f, "%.1f",
                       ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("Specular Strength", &m_specularStrength, 0.0f, 1.0f);
    ImGui::Separator();
    ImGui::Text("Sky Shader Settings");
    ImGui::SliderFloat("Cloud Cover", &m_cloudCover, 0.0f, 1.0f);
    ImGui::SliderFloat("Cloud Falloff", &m_cloudFalloff, 0.0f, 1.0f);
    if (m_cloudFalloff < m_cloudCover) {
      m_cloudFalloff = m_cloudCover + 0.01f;
    }
    auto &postProcessing = m_engine->GetPostProcessing();
    if (ImGui::BeginCombo(
            "Post-Processing Shader",
            postproShaderText[postProcessing.GetCurrentShaderIndex()])) {
      for (int i = 0; i < 13; i++) {
        const bool is_selected = (postProcessing.GetCurrentShaderIndex() == i);
        if (ImGui::Selectable(postproShaderText[i], is_selected)) {
          postProcessing.SetShader((PostproShader)i);
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::Checkbox("Enable Post-Processing", &m_postProcessingActive);
    ImGui::Separator();
    ImGui::Text("spawn a bull");
    if (ImGui::Button("spawn")) {
      auto newBull =
          std::make_shared<bulls>(&physicsSystem, assetLoader, m_playerSheep);
      newBull->name = "Bull_AI";
      newBull->model.materials[0].shader = renderer.GetShadowShader();
      gameObjectManager.AddGameObject(newBull);
      Shader shadowShader = renderer.GetShadowShader();
      newBull->AssignShader(shadowShader);
    }
    ImGui::End();

    EditorUI::RenderHierarchy(gameObjectManager, physicsSystem,
                              renderer.GetShadowShader(), m_progress,
                              assetLoader);
  }
  rlImGuiEnd();
}

void GameScene::Shutdown() {
  // All systems are now managed by the Engine, so we don't delete them here.
  delete[] m_convertedIndices;
  delete m_indexVertexArray;
  UnloadModel(m_testmap);
  UnloadModel(m_sphereModel);
}
