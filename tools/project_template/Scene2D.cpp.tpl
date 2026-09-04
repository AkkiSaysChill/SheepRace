#include "game/%%SCENE_NAME%%.h"
#include "engine/Engine.h"
#include "raylib.h"

%%SCENE_NAME%%::%%SCENE_NAME%%() : m_engine(nullptr) {}

%%SCENE_NAME%%::~%%SCENE_NAME%%() {
    Shutdown();
}

void %%SCENE_NAME%%::Initialize(Engine* engine) {
    Scene2D::Initialize(engine);
    m_engine = engine;
}

void %%SCENE_NAME%%::Update(float deltaTime) {
    // 2D gameplay/UI update logic
}

void %%SCENE_NAME%%::Draw() {
    DrawText("This is a 2D Scene!", 20, 20, 20, RAYWHITE);
    DrawRectangle(GetScreenWidth() / 2 - 50, GetScreenHeight() / 2 - 50, 100, 100, MAROON);
}

void %%SCENE_NAME%%::Shutdown() {}