#pragma once

#include "Water.h"
#include "engine/PhysicsComponent.h"
#include "engine/Scene3D.h"
#include <memory>

class TheFresnelEffectScene : public Scene3D {
public:
  TheFresnelEffectScene();
  ~TheFresnelEffectScene() override;

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw() override;
  void Draw3D() override;
  void Shutdown() override;

private:
  Engine *m_engine;
  Model m_testmap;
  std::shared_ptr<Water> m_water;
  Shader m_customTerrainShader;

  btRigidBody *m_playerBody;
  float m_cameraYaw = 0.0f;
  float m_cameraPitch = 0.0f;
};
