#pragma once
#include "engine/Scene3D.h"
#include "raylib.h"

class Engine;

class MainMenuScene : public Scene3D {
public:
  MainMenuScene();
  virtual ~MainMenuScene();

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw3D() override;
  void Draw() override;
  void Shutdown() override;

private:
  Engine *m_engine = nullptr;
  int m_selectedOption = 0;
  const int m_totalOptions = 3;

  bool DrawButton(const char *text, Rectangle rect, bool isSelected,
                  float alpha);

  // Intro Sequence State
  float m_introTimer = 0.0f;
  float m_menuAlpha = 0.0f;
  float m_screenShake = 0.0f;
  Vector3 m_planePos = {0};
  Model m_cargoModel = {0};
  bool m_cargoLoaded = false;

  Sound m_sndFlyby = {0};
  bool m_playedFlyby = false;
};
