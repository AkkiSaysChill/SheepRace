#pragma once

#include "engine/PlayerProgress.h"
#include "engine/Scene3D.h"
#include "raylib.h"
#include <string>
#include <vector>

class Engine;
class Ocean;

class MissionSelectScene : public Scene3D {
public:
  MissionSelectScene();
  ~MissionSelectScene() override;

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw3D() override;
  void Draw() override;
  void Shutdown() override;

private:
  Engine *m_engine;
  PlayerProgress m_progress;
  int m_selectedIndex;
  int m_scrollOffset;
  Font m_customFont;
  bool m_fontLoaded;

  // Moved from static globals to instance members
  Ocean *m_ocean = nullptr;
  float m_missionCamAngle = 0.0f;

  void EnsureProgressSynchronized();
  bool DrawMissionCard(int index, const std::string &missionId, Rectangle rect,
                       bool isSelected);
  void DrawUIString(const char *text, float posX, float posY, float fontSize,
                    Color color);
  float MeasureUIString(const char *text, float fontSize);
};
