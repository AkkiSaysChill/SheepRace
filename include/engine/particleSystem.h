#pragma once
#include "raylib.h"
#include "raymath.h"
#include <memory>
#include <vector>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// ============================================================
// EMISSION MODE: Controls how an emitter fires particles
// ============================================================
enum class EmissionMode {
  BURST,     // Fires a fixed count in one frame (sparks, shockwave, splash)
  CONTINUOUS // Fires N particles per second (smoke, rain)
};

// ============================================================
// BILLBOARD MODE: Controls how quads orient themselves
// ============================================================
enum class BillboardMode {
  SCREEN_ALIGNED, // Always faces camera fully (sparks, smoke)
  Y_LOCKED        // Only rotates around Y axis (shockwave ring on ground plane)
};

// ============================================================
// EMITTER CONFIG: Data-driven. No subclasses needed.
// ============================================================
struct EmitterConfig {
  // --- Pool ---
  int maxParticles = 500;

  // --- Emission ---
  EmissionMode emissionMode = EmissionMode::BURST;
  float emissionRate = 0.0f; // Particles/sec (CONTINUOUS only)

  // --- Lifetime ---
  Vector2 lifetimeRange = {1.0f, 2.0f};

  // --- Spawn Shape ---
  Vector3 spawnRadius = {0.0f, 0.0f, 0.0f};

  // --- Motion ---
  Vector2 speedRange = {1.0f, 5.0f};
  Vector3 gravity = {0.0f, -9.8f, 0.0f};
  float drag = 0.0f; // 0 = no drag, 0.98 = heavy air resistance (applied as vel
                     // *= (1-drag*dt))

  // --- Visuals ---
  Color startColor = WHITE;
  Color endColor = {255, 255, 255, 0};
  float startSize = 1.0f;
  float endSize = 0.0f;
  float isRing = 0.0f;
  float startRotation = 0.0f;  // radians
  float rotationSpeed = 0.0f;  // radians/sec (random sign applied per particle)
  bool randomRotation = false; // randomize starting angle

  // --- Rendering ---
  BillboardMode billboard = BillboardMode::SCREEN_ALIGNED;
};

// ============================================================
// SIM DATA: Pure SoA layout for cache-friendly CPU update
// ============================================================
struct ParticleSimData {
  std::vector<Vector3> velocities;
  std::vector<float> lifeTimes;
  std::vector<float> maxLives;
  std::vector<float> rotations; // current rotation angle (radians)
  std::vector<float> rotSpeeds; // per-particle rotation speed
  std::vector<int> activeFlags; // 0 = dead, 1 = alive
};

// ============================================================
// EMITTER: Manages a pool of particles under one config
// ============================================================
class Emitter {
public:
  EmitterConfig config;
  int maxCapacity = 0;
  int lastUsedParticle = 0;

  // Continuous emission accumulator
  float m_emitAccumulator = 0.0f;

  ParticleSimData simData;

  // Render data: packed into Matrix for DrawMeshInstanced
  // Layout per-matrix:
  //   col0: (pos.x, pos.y, pos.z, size)
  //   col1: (r, g, b, a)          [0..1 floats]
  //   col2: (rotation, 0, 0, 0)   [radians, billboard hint]
  std::vector<Matrix> renderTransforms;

  // Pre-allocated active index list — rebuilt each frame, no heap alloc
  std::vector<int> activeIndices;
  int activeCount = 0;

  void Initialize(EmitterConfig cfg);

  // Emit a burst of `count` particles from `origin`
  void Emit(int count, Vector3 origin);

  // Call every frame; handles continuous emission internally
  void Update(float dt, Vector3 continuousOrigin = {0, 0, 0});

private:
  int FindNextDeadParticle();
  void SpawnOne(Vector3 origin);
};

// ============================================================
// PARTICLE SYSTEM: Container + GPU renderer
// ============================================================
class ParticleSystem {
public:
  ~ParticleSystem();

  void Initialize(Texture2D effectTex, Shader instancedShader);
  std::shared_ptr<Emitter> AddEmitter(EmitterConfig config);

  void Update(float deltaTime);
  void Draw(Camera3D cam);

  // Optional: Remove a specific emitter at runtime
  void RemoveEmitter(std::shared_ptr<Emitter> emitter);

private:
  std::vector<std::shared_ptr<Emitter>> m_emitters;

  Mesh m_quadMesh = {};
  Material m_material = {};

  int m_viewLoc = -1;
  int m_billboardLoc = -1; // uniform int: 0=screen, 1=y-locked

  // Single reusable buffer — reserved once, never reallocated mid-frame
  std::vector<Matrix> m_instanceBuffer;
};

// ============================================================
// PRESET FACTORY (inline, header-only for convenience)
// ============================================================
namespace ParticlePresets {

inline EmitterConfig Smoke() {
  EmitterConfig c;
  c.maxParticles = 500;
  c.emissionMode = EmissionMode::CONTINUOUS;
  c.emissionRate = 15.0f;
  c.gravity = {0.0f, 3.0f, 0.0f};
  c.lifetimeRange = {1.5f, 3.0f};
  c.speedRange = {0.5f, 2.0f};
  c.spawnRadius = {0.5f, 0.2f, 0.5f};
  c.drag = 1.5f;
  c.startColor = {100, 100, 100, 200};
  c.endColor = {50, 50, 50, 0};
  c.startSize = 1.0f;
  c.endSize = 6.0f;
  c.randomRotation = true;
  c.rotationSpeed = 0.3f;
  return c;
}

inline EmitterConfig Sparks() {
  EmitterConfig c;
  c.maxParticles = 1000;
  c.emissionMode = EmissionMode::BURST;
  c.gravity = {0.0f, -25.0f, 0.0f};
  c.lifetimeRange = {0.1f, 0.4f};
  c.speedRange = {15.0f, 35.0f};
  c.spawnRadius = {0.0f, 0.0f, 0.0f};
  c.drag = 0.5f;
  c.startColor = {255, 200, 0, 255};
  c.endColor = {255, 0, 0, 0};
  c.startSize = 0.4f;
  c.endSize = 0.0f;
  return c;
}

inline EmitterConfig Rain() {
  EmitterConfig c;
  c.maxParticles = 2000;
  c.emissionMode = EmissionMode::CONTINUOUS;
  c.emissionRate = 400.0f;
  c.gravity = {0.0f, -40.0f, 0.0f};
  c.lifetimeRange = {1.0f, 1.5f};
  c.speedRange = {0.0f, 2.0f};
  c.spawnRadius = {40.0f, 0.0f, 40.0f};
  c.startColor = {150, 200, 255, 100};
  c.endColor = {150, 200, 255, 0};
  c.startSize = 0.2f;
  c.endSize = 0.2f;
  return c;
}

inline EmitterConfig WaterSplash() {
  EmitterConfig c;
  c.maxParticles = 500;
  c.emissionMode = EmissionMode::BURST;
  c.gravity = {0.0f, -15.0f, 0.0f};
  c.lifetimeRange = {0.3f, 0.6f};
  c.speedRange = {5.0f, 12.0f};
  c.spawnRadius = {0.5f, 0.0f, 0.5f};
  c.drag = 1.0f;
  c.startColor = {100, 200, 255, 200};
  c.endColor = {255, 255, 255, 0};
  c.startSize = 0.5f;
  c.endSize = 0.1f;
  return c;
}

inline EmitterConfig Shockwave() {
  EmitterConfig c;
  c.maxParticles = 20;
  c.emissionMode = EmissionMode::BURST;
  c.gravity = {0.0f, 0.0f, 0.0f};
  c.lifetimeRange = {0.4f, 0.4f};
  c.speedRange = {0.0f, 0.0f};
  c.spawnRadius = {0.0f, 0.0f, 0.0f};
  c.startColor = {255, 255, 255, 150};
  c.endColor = {255, 255, 255, 0};
  c.startSize = 0.5f;
  c.endSize = 15.0f;
  c.billboard = BillboardMode::Y_LOCKED; // ring lies flat on the ground

  c.isRing = 1.0f;
  return c;
}

} // namespace ParticlePresets
