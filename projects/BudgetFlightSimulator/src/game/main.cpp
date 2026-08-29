#include "engine/Engine.h"
#include "game/BudgetFlightSimulatorScene.h"
#include "game/HangarSelectScene.h"
#include "game/MainMenuScene.h"
#include <memory>

void RegisterGameScenes(Engine &engine) {
  auto &sceneManager = engine.GetSceneManager();

  // Register scenes
  sceneManager.AddScene<MainMenuScene>("MainMenuScene");
  sceneManager.AddScene<HangarSelectScene>("HangarScene");
  sceneManager.AddScene<BudgetFlightSimulatorScene>("FlightScene");

  // Launch directly into the Main Menu
  sceneManager.LoadScene("MainMenuScene");
}
