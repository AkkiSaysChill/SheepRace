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
#include <unordered_map>
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
  void TriggerExplosionFX(Vector3 pos, float intensity = 1.0f);

  // HUD Render Subsystems
  void DrawCompassTape(float headingDeg, int screenWidth);
  void DrawAltitudeBar(float altitudeM, float groundHeightM, int screenWidth,
                       int screenHeight);
  void DrawPitchLadderHUD(float pitchDeg, float rollDeg, int screenHeight);
  void DrawTelemetryCluster(int screenWidth, int screenHeight);
  // Radar Subsystem
  void DrawRadarHUD(int screenWidth, int screenHeight);

  // Text helpers for custom font
  void DrawUIString(const char *text, float posX, float posY, float fontSize,
                    Color color);
  float MeasureUIString(const char *text, float fontSize);
  int DrawTextWordWrap(const char *text, int posX, int posY, int maxWidth,
                       int fontSize, int lineSpacing, Color color);

  // Procedural Aircraft Animation Helpers
  void DrawAnimatedAircraftModel(const Model &model, Vector3 position,
                                 Quaternion orientation,
                                 const std::string &planeId);
  void DrawProceduralPropeller(Vector3 hubPos, float spinAngleDeg,
                               Matrix planeWorldMatrix);

  Engine *m_engine = nullptr;
  std::shared_ptr<GameObject> m_planeObject;

  TerrainMap m_terrain;
  Ocean m_ocean;

  bool m_isFrozen = false;

  MissionManager m_missionManager;

  Aircraft m_aircraft;
  FlightCamera m_flightCamera;

  // Cached airframe dictionary (eliminates runtime heap alloc/dealloc)
  std::unordered_map<std::string, Model> m_loadedAircraftModels;
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

  // Enemy Jet Model
  Model m_enemyJetModel;
  bool m_enemyJetModelLoaded = false;

  // Scene-owned Particle System
  ParticleSystem m_particleSystem;
  Texture2D m_particleTexture = {0};
  Shader m_particleShader = {0};
  bool m_particlesInitialized = false;

  // Custom HUD Font
  Font m_hudFont = {0};
  bool m_customFontLoaded = false;

  // Particle Emitters
  std::shared_ptr<Emitter> m_explosionEmitter;
  std::shared_ptr<Emitter> m_missileSmokeEmitter;
  std::shared_ptr<Emitter> m_bulletSparkEmitter;
  std::shared_ptr<Emitter> m_runwaySmokeEmitter;
  std::shared_ptr<Emitter> m_shockwaveEmitter;

  // Propeller / Fan animation state
  float m_propellerAngleDeg = 0.0f;

  // Post-Processing & Screen Shake Timers
  float m_explosionPostFXTimer = 0.0f;
  float m_screenShakeIntensity = 0.0f;

  // Emergency Engine Failure State
  bool m_engineFailed = false;
  float m_engineSmokeTimer = 0.0f;
};
