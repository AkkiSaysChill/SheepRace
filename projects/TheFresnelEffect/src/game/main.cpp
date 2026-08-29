#include "engine/Engine.h"
#include "game/TheFresnelEffectScene.h"
#include <memory>

// This function is called by the main engine to register this game's scenes
void RegisterGameScenes(Engine& engine) {
    auto& sceneManager = engine.GetSceneManager();

    // Add your scenes here
    sceneManager.AddScene<TheFresnelEffectScene>("DefaultScene");

    // Load the initial scene for the game
    sceneManager.LoadScene("DefaultScene");
}
