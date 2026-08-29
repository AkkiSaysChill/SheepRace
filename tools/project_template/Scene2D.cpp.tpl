#include "%%SCENE_NAME%%.h"
#include "engine/Engine.h"
#include "raylib.h"

%%SCENE_NAME%%::%%SCENE_NAME%%() : m_engine(nullptr) {}

%%SCENE_NAME%%::~%%SCENE_NAME%%() {}

void %%SCENE_NAME%%::Initialize(Engine* engine) {
    // This call is important to set up the 2D camera
    Scene2D::Initialize(engine);
    m_engine = engine;

    // Get systems from the engine
    auto& gameObjectManager = m_engine->GetGameObjectManager();

    // In a 2D scene, you would typically work with sprites or UI elements
}

void %%SCENE_NAME%%::Update(float deltaTime) {
    // Add your scene's update logic here
}

void %%SCENE_NAME%%::Draw() {
    // Add your scene's drawing logic here
    DrawText("This is a 2D Scene!", 10, 10, 20, DARKGRAY);
    DrawRectangle(GetScreenWidth() / 2 - 50, GetScreenHeight() / 2 - 50, 100, 100, MAROON);
}

void %%SCENE_NAME%%::Shutdown() {
    // Add your scene's shutdown logic here
}
