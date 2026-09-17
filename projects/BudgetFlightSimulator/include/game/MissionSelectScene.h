#pragma once

#include "engine/PlayerProgress.h"
#include "engine/Scene.h"
#include "raylib.h"
#include <string>
#include <vector>

class Engine;

class MissionSelectScene : public Scene {
public:
  MissionSelectScene();
  ~MissionSelectScene() override;

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw() override;
  void Shutdown() override;

private:
  Engine *m_engine;
  PlayerProgress m_progress;
  int m_selectedIndex;
  int m_scrollOffset;
  Font m_customFont;
  bool m_fontLoaded;

  void EnsureProgressSynchronized();
  bool DrawMissionCard(int index, const std::string &missionId, Rectangle rect,
                       bool isSelected);
};
