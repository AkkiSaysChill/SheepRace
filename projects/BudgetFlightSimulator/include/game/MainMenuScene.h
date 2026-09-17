#pragma once
#include "engine/Scene2D.h"
#include "raylib.h"

class Engine;

class MainMenuScene : public Scene2D {
public:
  MainMenuScene();
  virtual ~MainMenuScene();

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw() override;
  void Shutdown() override;

private:
  Engine *m_engine = nullptr;
  int m_selectedOption = 0;
  const int m_totalOptions = 4;

  bool DrawButton(const char *text, Rectangle rect, bool isSelected);
};
