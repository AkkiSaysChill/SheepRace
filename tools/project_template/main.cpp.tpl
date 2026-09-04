#include "engine/Engine.h"
#include "game/%%SCENE_NAME%%.h"
#include <memory>

void RegisterGameScenes(Engine& engine) {
    auto& sceneManager = engine.GetSceneManager();

    // Register Default Scene
    sceneManager.AddScene<%%SCENE_NAME%%>("DefaultScene");

    // Load Default Scene
    sceneManager.LoadScene("DefaultScene");
}