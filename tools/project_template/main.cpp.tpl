#include "engine/Engine.h"
#include "game/%%SCENE_NAME%%.h"
#include <memory>

// This function is called by the main engine to register this game's scenes
void RegisterGameScenes(Engine& engine) {
    auto& sceneManager = engine.GetSceneManager();

    // Add your scenes here
    sceneManager.AddScene<%%SCENE_NAME%%>("DefaultScene");

    // Load the initial scene for the game
    sceneManager.LoadScene("DefaultScene");
}
