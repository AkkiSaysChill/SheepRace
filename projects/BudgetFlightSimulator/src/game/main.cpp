#include "engine/Engine.h"
#include "game/BudgetFlightSimulatorScene.h"
#include "game/ComicScene.h"
#include "game/HangarSelectScene.h"
#include "game/LoadingScene.h"
#include "game/MainMenuScene.h"
#include "game/MissionData.h"
#include "game/MissionSelectScene.h"
#include <memory>

void RegisterGameScenes(Engine &engine) {
  auto &sceneManager = engine.GetSceneManager();

  // Register scenes
  sceneManager.AddScene<MainMenuScene>("MainMenuScene");
  sceneManager.AddScene<HangarSelectScene>("HangarScene");
  sceneManager.AddScene<BudgetFlightSimulatorScene>("FlightScene");
  sceneManager.AddScene<MissionSelectScene>("MissionSelectScene");
  sceneManager.AddScene<LoadingScene>("LoadingScene");
  sceneManager.AddScene<ComicScene>("ComicScene");

  SetWindowTitle("Budget Flight Simulator");

  // Launch directly into the Main Menu
  sceneManager.LoadScene("MainMenuScene");
}
