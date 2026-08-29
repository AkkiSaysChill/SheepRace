#include "engine/Engine.h"
#include "game/MainMenuScene.h"
#include "game/GameScene.h"
#include <memory>

void RegisterGameScenes(Engine& engine) {
    auto& sceneManager = engine.GetSceneManager();

    // Add scenes using the new, simpler templated method
    sceneManager.AddScene<MainMenuScene>("MainMenu");
    sceneManager.AddScene<GameScene>("GameScene");

    // Load the initial scene for the game
    sceneManager.LoadScene("MainMenu");
}
