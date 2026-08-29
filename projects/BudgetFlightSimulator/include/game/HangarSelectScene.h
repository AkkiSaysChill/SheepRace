#pragma once
#include "engine/Scene3D.h"
#include "game/Aircraft.h"
#include "raylib.h"
#include <string>
#include <vector>

class Engine;

class HangarSelectScene : public Scene3D {
public:
  HangarSelectScene();
  virtual ~HangarSelectScene();

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw3D() override;
  void Draw() override;
  void Shutdown() override;

private:
  void LoadCurrentAircraft();
  void DrawStatBar(const char *label, float value, float maxValue, int x, int y,
                   int width);

  Engine *m_engine = nullptr;
  Camera3D m_camera = {0};

  Model m_currentModel;
  bool m_modelLoaded = false;

  std::vector<std::string> m_planeIds;
  size_t m_selectedIndex = 0;
  AircraftStats m_currentStats;

  float m_rotationAngle = 0.0f;
  float m_camDistance = 18.0f;
  float m_camHeight = 4.0f;
};
