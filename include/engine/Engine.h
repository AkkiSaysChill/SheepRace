#pragma once

#include "engine/GameObjectManager.h"
#include "engine/PhysicsSystem.h"
#include "engine/PostProcessing.h"
#include "engine/Renderer.h"
#include "engine/SceneManager.h"
#include "engine/TextManager.h"
#include "engine/asset_loader.hpp"
#include "raylib.h"
#include "engine/DayNightCycle.h"
#include "engine/WeatherSystem.h"

class Engine {
public:
  Engine();
  ~Engine();

  void Initialize();
  void Run();
  void Shutdown();

  // Getters for core systems
  SceneManager &GetSceneManager() { return m_sceneManager; }
  AssetPacking::AssetLoader &GetAssetLoader() { return m_assetLoader; }
  GameObjectManager &GetGameObjectManager() { return m_gameObjectManager; }
  PhysicsSystem &GetPhysicsSystem() { return m_physicsSystem; }
  Renderer &GetRenderer() { return *m_renderer; }
  PostProcessing &GetPostProcessing() { return *m_postProcessing; }
  Camera &GetCamera() { return m_camera; }

  TextManager &GetTextManager() { return m_textManager; }
  bool PostProcessingActive = false;

  DayNightCycle& GetDayNightCycle() { return m_dayNightCycle; }

  WeatherSystem& GetWeatherSystem() { return m_weatherSystem; }

private:
  int m_screenWidth;
  int m_screenHeight;
  
  DayNightCycle m_dayNightCycle;
  WeatherSystem m_weatherSystem;

  TextManager m_textManager;

  void resize();

  // Core Engine Systems
  AssetPacking::AssetLoader m_assetLoader;
  SceneManager m_sceneManager;
  GameObjectManager m_gameObjectManager;
  PhysicsSystem m_physicsSystem;
  Renderer *m_renderer;
  PostProcessing *m_postProcessing;
  Camera m_camera;
};
