#pragma once

#include "engine/Scene3D.h"
#include "engine/particleSystem.h"
#include "game/Aircraft.h"
#include "game/CombatEntities.h"
#include "game/FlightCamera.h"
#include "game/MissionManager.h"
#include "game/Ocean.h"
#include "game/TerrainMap.h"
#include "raylib.h"
#include <memory>
#include <string>
#include <vector>

class GameObject;

struct DroppedPayload {
    Vector3 position;
    Vector3 velocity;
    bool active;
    bool inWater;
};

class BudgetFlightSimulatorScene : public Scene3D {
public:
  BudgetFlightSimulatorScene();
  virtual ~BudgetFlightSimulatorScene();

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw3D() override;
  void Draw() override;
  void Shutdown() override;

  void SwitchAircraft(const std::string &planeId);

private:
  void UpdateDogfight(float deltaTime);
  void UpdateProjectiles(float deltaTime);

  Engine *m_engine = nullptr;
  std::shared_ptr<GameObject> m_planeObject;

  TerrainMap m_terrain;
  Ocean m_ocean;

  bool m_isFrozen = false;

  MissionManager m_missionManager;

  Aircraft m_aircraft;
  FlightCamera m_flightCamera;

  Model m_planeModel;
  bool m_modelLoaded = false;
  float m_throttle = 0.5f;

  size_t m_currentPlaneIndex = 0;

  Shader m_worldFogShader = {0};
  bool m_worldFogLoaded = false;

  std::vector<DroppedPayload> m_droppedPayloads;
  Model m_cargoModel;
  bool m_cargoModelLoaded = false;

  // Combat Entities
  std::vector<BulletTracer> m_bullets;
  std::vector<HomingMissile> m_missiles;
  std::vector<EnemyAce> m_enemyJets;

  // Enemy Jet Model (Same jet_mk2 tinted in enemy red)
  Model m_enemyJetModel;
  bool m_enemyJetModelLoaded = false;

  // Particle System & Texture/Shader handles
  ParticleSystem m_particleSystem;
  Texture2D m_particleTexture = {0};
  Shader m_particleShader = {0};
  bool m_particlesInitialized = false;

  // Particle Emitter References
  std::shared_ptr<Emitter> m_explosionEmitter;
  std::shared_ptr<Emitter> m_missileSmokeEmitter;
  std::shared_ptr<Emitter> m_bulletSparkEmitter;
};