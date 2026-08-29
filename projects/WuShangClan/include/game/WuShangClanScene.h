#pragma once

#include "engine/LevelEditor.h"
#include "engine/Scene3D.h"
#include "engine/VisualLogicEditor.h"
#include "engine/particleSystem.h"
#include "game/ChatSystem.h"
#include "game/weapon.h"
#include "raylib.h"
#include <memory>

class WuShangClanScene : public Scene3D {
public:
  WuShangClanScene();
  ~WuShangClanScene() override;

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw() override;
  void Draw3D() override;
  void Shutdown() override;

private:
  Engine *m_engine;
  Model m_brokenPillarModel;
  std::shared_ptr<Weapon> m_arsenal;

  LevelEditor m_levelEditor;

  VisualLogicEditor m_logicEditor;

  ChatSystem m_chatSystem;

  // player related shit
  btRigidBody *m_playerBody = nullptr;
  float m_cameraYaw = 0.0f;
  float m_cameraPitch = 0.0f;
  bool m_isSliding = false;

  bool m_tool_active = false;

  ParticleSystem globalVFX;
  std::shared_ptr<Emitter> emitterSparks;
  std::shared_ptr<Emitter> emitterSmoke;
  std::shared_ptr<Emitter> emitterRain;
  std::shared_ptr<Emitter> emitterSplash;
  std::shared_ptr<Emitter> emitterShockwave;

  // Hollow Mesh Physics Trackers (To prevent memory leaks)
  int *m_convertedIndices = nullptr;
  btTriangleIndexVertexArray *m_indexVertexArray = nullptr;

  Color m_levelColor = PURPLE;
  Vector3 m_bulbPosition = {0.0f, 23.0f, 0.0f};
  std::shared_ptr<GameObject> m_roofLight;

  // camera shake
  float m_shakeTimer = 0.0f;
  float m_shakeIntensity = 0.0f;
};
