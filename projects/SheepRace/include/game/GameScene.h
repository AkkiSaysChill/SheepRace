#pragma once

#include "engine/LevelEditor.h"
#include "engine/PlayerProgress.h"
#include "engine/Scene3D.h"
#include "engine/asset_loader.hpp"
#include "game/bulls.h"
#include "game/playerSheep.h"
#include <memory>
#include <vector>

class Engine;
class ParticleSystem;

class GameScene : public Scene3D {
public:
  GameScene();
  ~GameScene() override;

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw() override;
  void Shutdown() override;
  void OnResize(int width, int height) override;

private:
  Engine *m_engine; // Pointer to the engine to access core systems
  Car *m_playerSheep;
  std::vector<bulls> m_bulls;

  LevelEditor m_levelEditor;

  // Game specific variables
  bool m_tool_active;
  float m_fov;
  float m_shininess;
  float m_specularStrength;
  Vector3 m_lightDir;
  bool m_postProcessingActive;
  float m_cloudCover;
  float m_cloudFalloff;
  float m_lightCamOrthoSize;
  float m_cameraMouseMoveSensitivity;

  float m_cameraYaw = 0.0f;
  float m_cameraPitch = 0.0f;

  // testing particle particle system
  std::shared_ptr<ParticleSystem> m_smoke;
  float m_gravity = 10;
  float m_tempG = m_gravity;

  Model m_testmap;
  Model m_sphereModel;
  btTriangleIndexVertexArray *m_indexVertexArray;
  int *m_convertedIndices;

  PlayerProgress m_progress;
};
