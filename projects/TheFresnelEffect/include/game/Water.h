#pragma once
#include "engine/GameObject.h"
#include "raylib.h"

class Engine;

class Water : public GameObject {
public:
  Water(Engine *engine, Vector3 position, Vector2 size);
  ~Water();

  void Update(float deltaTime) override;

  Model &GetModel() { return m_waterModel; }

private:
  Engine *m_engine;
  Model m_waterModel; // We need to store the model so we can update its mesh!
  float m_time = 0.0f;
  int m_cameraPosLoc = -1;
  int m_scrollLoc = -1;
  int m_nearLoc = -1;
  int m_farLoc = -1;
  int m_mvpLoc = -1;
  Shader m_fresnelShader;
};
