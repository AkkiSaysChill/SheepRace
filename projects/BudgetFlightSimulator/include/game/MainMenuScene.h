#pragma once
#include "engine/Scene3D.h"
#include "raylib.h"

class Engine;
class Ocean;

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

  float m_introTimer = 0.0f;
  float m_menuAlpha = 0.0f;
  float m_screenShake = 0.0f;
  Vector3 m_planePos = {0};
  Model m_cargoModel = {0};
  bool m_cargoLoaded = false;

  Sound m_sndFlyby = {0};
  bool m_playedFlyby = false;

  // Moved from static globals to instance members
  Ocean *m_ocean = nullptr;
  Font m_mainFont = {0};
  bool m_mainFontLoaded = false;

  void DrawUIStringMenu(const char *text, float posX, float posY,
                        float fontSize, Color color, float alphaMulti);
  float MeasureUIStringMenu(const char *text, float fontSize);
};
