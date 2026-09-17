#include "engine/Engine.h"
#include "game/BudgetFlightSimulatorScene.h"
#include "game/HangarSelectScene.h"
#include "game/MainMenuScene.h"
#include "game/MissionSelectScene.h"
#include <memory>

void RegisterGameScenes(Engine &engine) {
  auto &sceneManager = engine.GetSceneManager();

  // Register scenes
  sceneManager.AddScene<MainMenuScene>("MainMenuScene");
  sceneManager.AddScene<HangarSelectScene>("HangarScene");
  sceneManager.AddScene<BudgetFlightSimulatorScene>("FlightScene");
  sceneManager.AddScene<MissionSelectScene>("MissionSelectScene");

  SetWindowTitle("Budget Flight Simulator");

  // Launch directly into the Main Menu
  sceneManager.LoadScene("MainMenuScene");
}
