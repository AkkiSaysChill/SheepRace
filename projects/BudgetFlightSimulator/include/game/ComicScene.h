#pragma once
#include "engine/Scene2D.h"
#include "raylib.h"
#include <string>
#include <vector>

class Engine;

class ComicScene : public Scene2D {
public:
  ComicScene();
  ~ComicScene() override;

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw() override;
  void Shutdown() override;

private:
  Engine *m_engine = nullptr;

  int m_currentPanelIndex = 0;
  float m_panelTimer = 0.0f;
  float m_fadeAlpha = 1.0f;
  bool m_isFadingOut = false;

  Texture2D m_currentImage = {0};
  Sound m_currentVoiceover = {0};
  Font m_comicFont = {0};

  void LoadCurrentPanel();
  void UnloadCurrentPanel();
};
