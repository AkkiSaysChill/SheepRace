#include "engine/Engine.h"
#include "game/VolumetricCloudsScene.h"
#include <memory>

void RegisterGameScenes(Engine& engine) {
    auto& sceneManager = engine.GetSceneManager();

    // Register Default Scene
    sceneManager.AddScene<VolumetricCloudsScene>("DefaultScene");

    // Load Default Scene
    sceneManager.LoadScene("DefaultScene");
}