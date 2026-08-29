#include "game/%%SCENE_NAME%%.h"
#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/DrawableComponent.h"
#include "engine/PhysicsComponent.h"
#include "raylib.h"
#include "raymath.h"

%%SCENE_NAME%%::%%SCENE_NAME%%() : m_engine(nullptr) {}

%%SCENE_NAME%%::~%%SCENE_NAME%%() {}

void %%SCENE_NAME%%::Initialize(Engine* engine) {
    // This call is important to set up the 3D camera
    Scene3D::Initialize(engine);
    m_engine = engine;

    // Get systems from the engine
    auto& gameObjectManager = m_engine->GetGameObjectManager();

    // --- Create a sample cube ---
    auto cube = std::make_shared<GameObject>();
    cube->name = "Sample Cube";
    cube->position = {0, 1, 0};
    
    // Add a drawable component
    Model cubeModel = LoadModelFromMesh(GenMeshCube(2.0f, 2.0f, 2.0f));
    cube->AddComponent<DrawableComponent>(cubeModel);

    // Add a physics component
    btCollisionShape* shape = new btBoxShape(btVector3(1.0f, 1.0f, 1.0f));
    cube->AddComponent<PhysicsComponent>(&m_engine->GetPhysicsSystem(), shape, PhysicsShapeType::BOX, 1.0f, cube->position, QuaternionIdentity());

    gameObjectManager.AddGameObject(cube);
}

void %%SCENE_NAME%%::Update(float deltaTime) {
    // Add your scene's update logic here
}

void %%SCENE_NAME%%::Draw() {
    // Add your scene's drawing logic here (e.g., ImGui UI, debug draws)
    DrawText("This is a 3D Scene!", 10, 10, 20, DARKGRAY);
}

void %%SCENE_NAME%%::Shutdown() {
    // Add your scene's shutdown logic here
}
