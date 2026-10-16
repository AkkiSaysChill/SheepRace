#pragma once
#include "engine/Scene2D.h"
#include "raylib.h"
#include <string>

class Engine;

class LoadingScene : public Scene2D {
public:
  LoadingScene();
  ~LoadingScene() override;

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw() override;
  void Shutdown() override;

private:
  Engine *m_engine = nullptr;
  int m_framesWaited = 0;
  Font m_customFont = {0};
  bool m_fontLoaded = false;
  float m_spinnerAngle = 0.0f;
  std::string m_randomTip;
};
