#pragma once
#include "engine/Scene3D.h"
#include "game/Aircraft.h"
#include "game/FlightCamera.h"
#include "game/Ocean.h"
#include "game/TerrainMap.h"
#include "raylib.h"
#include <memory>
#include <string>

class GameObject;

class BudgetFlightSimulatorScene : public Scene3D {
public:
  BudgetFlightSimulatorScene();
  virtual ~BudgetFlightSimulatorScene();

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw3D() override;
  void Draw() override;
  void Shutdown() override;

  void SwitchAircraft(const std::string &planeId);

private:
  Engine *m_engine = nullptr;
  std::shared_ptr<GameObject> m_planeObject;

  TerrainMap m_terrain;
  Ocean m_ocean;

  bool m_isFrozen = false;

  Aircraft m_aircraft;
  FlightCamera m_flightCamera;

  Model m_planeModel;
  bool m_modelLoaded = false;
  float m_throttle = 0.5f;

  size_t m_currentPlaneIndex = 0;

  Shader m_worldFogShader = {0};
  bool m_worldFogLoaded = false;
};
