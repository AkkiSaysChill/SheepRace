#include "engine/particleSystem.h"
#include "raylib.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

// ============================================================
// Helper Math
// ============================================================
static float GetRandomFloat(float min, float max) {
  return min + (rand() / (float)RAND_MAX) * (max - min);
}

// Uniform sphere distribution
static Vector3 GetRandomDir() {
  float theta = GetRandomFloat(0.0f, 2.0f * PI);
  float phi = acosf(GetRandomFloat(-1.0f, 1.0f));
  return {sinf(phi) * cosf(theta), cosf(phi), sinf(phi) * sinf(theta)};
}

static Color ColorLerpCustom(Color c1, Color c2, float t) {
  return {(unsigned char)(c1.r + (c2.r - c1.r) * t),
          (unsigned char)(c1.g + (c2.g - c1.g) * t),
          (unsigned char)(c1.b + (c2.b - c1.b) * t),
          (unsigned char)(c1.a + (c2.a - c1.a) * t)};
}

// ============================================================
// Emitter :: Initialize
// ============================================================
void Emitter::Initialize(EmitterConfig cfg) {
  config = cfg;
  maxCapacity = cfg.maxParticles;

  simData.velocities.assign(maxCapacity, {0, 0, 0});
  simData.lifeTimes.assign(maxCapacity, 0.0f);
  simData.maxLives.assign(maxCapacity, 0.0f);
  simData.rotations.assign(maxCapacity, 0.0f);
  simData.rotSpeeds.assign(maxCapacity, 0.0f);
  simData.activeFlags.assign(maxCapacity, 0);

  renderTransforms.resize(maxCapacity, MatrixIdentity());

  // Pre-allocate active index list — avoids ANY heap allocation per frame
  activeIndices.resize(maxCapacity);
  activeCount = 0;
}

// ============================================================
// Emitter :: FindNextDeadParticle  (two-scan circular search)
// ============================================================
int Emitter::FindNextDeadParticle() {
  for (int i = lastUsedParticle; i < maxCapacity; i++) {
    if (!simData.activeFlags[i]) {
      lastUsedParticle = i;
      return i;
    }
  }
  for (int i = 0; i < lastUsedParticle; i++) {
    if (!simData.activeFlags[i]) {
      lastUsedParticle = i;
      return i;
    }
  }
  return -1; // pool full
}

// ============================================================
// Emitter :: SpawnOne  (internal helper)
// ============================================================
void Emitter::SpawnOne(Vector3 origin) {
  int idx = FindNextDeadParticle();
  if (idx == -1)
    return;

  simData.activeFlags[idx] = 1;
  simData.lifeTimes[idx] =
      GetRandomFloat(config.lifetimeRange.x, config.lifetimeRange.y);
  simData.maxLives[idx] = simData.lifeTimes[idx];

  float speed = GetRandomFloat(config.speedRange.x, config.speedRange.y);
  simData.velocities[idx] = Vector3Scale(GetRandomDir(), speed);

  // Rotation
  float baseRot = config.randomRotation ? GetRandomFloat(0.0f, 2.0f * PI)
                                        : config.startRotation;
  simData.rotations[idx] = baseRot;

  // Random sign on rotation speed for natural swirl
  float signedRotSpeed =
      config.rotationSpeed * (GetRandomFloat(0, 1) > 0.5f ? 1.0f : -1.0f);
  simData.rotSpeeds[idx] = signedRotSpeed;

  Vector3 offset = {
      GetRandomFloat(-config.spawnRadius.x, config.spawnRadius.x),
      GetRandomFloat(-config.spawnRadius.y, config.spawnRadius.y),
      GetRandomFloat(-config.spawnRadius.z, config.spawnRadius.z)};
  Vector3 startPos = Vector3Add(origin, offset);

  Matrix &m = renderTransforms[idx];
  // col0: pos.x, pos.y, pos.z, size
  m.m0 = startPos.x;
  m.m1 = startPos.y;
  m.m2 = startPos.z;
  m.m3 = config.startSize;
  // col1: r, g, b, a  (normalized)
  m.m4 = config.startColor.r / 255.0f;
  m.m5 = config.startColor.g / 255.0f;
  m.m6 = config.startColor.b / 255.0f;
  m.m7 = config.startColor.a / 255.0f;
  // col2: rotation, billboard mode
  m.m8 = baseRot;
  m.m9 = (config.billboard == BillboardMode::Y_LOCKED) ? 1.0f : 0.0f;
  m.m10 = config.isRing;
  m.m11 = 0.0f;
}

// ============================================================
// Emitter :: Emit  (burst)
// ============================================================
void Emitter::Emit(int count, Vector3 origin) {
  for (int i = 0; i < count; i++)
    SpawnOne(origin);
}

// ============================================================
// Emitter :: Update
// ============================================================
void Emitter::Update(float dt, Vector3 continuousOrigin) {
  // --- Continuous emission ---
  if (config.emissionMode == EmissionMode::CONTINUOUS &&
      config.emissionRate > 0.0f) {
    m_emitAccumulator += config.emissionRate * dt;
    int toSpawn = (int)m_emitAccumulator;
    m_emitAccumulator -= toSpawn;
    for (int i = 0; i < toSpawn; i++)
      SpawnOne(continuousOrigin);
  }

  // --- Simulate ---
  activeCount = 0;

  for (int i = 0; i < maxCapacity; i++) {
    if (!simData.activeFlags[i])
      continue;

    simData.lifeTimes[i] -= dt;
    if (simData.lifeTimes[i] <= 0.0f) {
      simData.activeFlags[i] = 0;
      continue;
    }

    // Drag (exponential decay approximation)
    if (config.drag > 0.0f) {
      float damping = 1.0f - config.drag * dt;
      if (damping < 0.0f)
        damping = 0.0f;
      simData.velocities[i] = Vector3Scale(simData.velocities[i], damping);
    }

    // Gravity
    simData.velocities[i] =
        Vector3Add(simData.velocities[i], Vector3Scale(config.gravity, dt));

    // Position integrate
    Vector3 pos = {renderTransforms[i].m0, renderTransforms[i].m1,
                   renderTransforms[i].m2};
    pos = Vector3Add(pos, Vector3Scale(simData.velocities[i], dt));

    // Rotation
    simData.rotations[i] += simData.rotSpeeds[i] * dt;

    // Lerp visuals
    float t = 1.0f - (simData.lifeTimes[i] / simData.maxLives[i]);
    float size = Lerp(config.startSize, config.endSize, t);
    Color col = ColorLerpCustom(config.startColor, config.endColor, t);

    // Pack back
    Matrix &m = renderTransforms[i];
    m.m0 = pos.x;
    m.m1 = pos.y;
    m.m2 = pos.z;
    m.m3 = size;
    m.m4 = col.r / 255.0f;
    m.m5 = col.g / 255.0f;
    m.m6 = col.b / 255.0f;
    m.m7 = col.a / 255.0f;
    m.m8 = simData.rotations[i];
    // m.m9 (billboard mode) is set at spawn time, stays constant

    activeIndices[activeCount++] = i;
  }
}

// ============================================================
// ParticleSystem :: ~ParticleSystem
// ============================================================
ParticleSystem::~ParticleSystem() {
  UnloadMaterial(m_material);
  UnloadMesh(m_quadMesh);
}

// ============================================================
// ParticleSystem :: Initialize
// ============================================================
void ParticleSystem::Initialize(Texture2D effectTex, Shader instancedShader) {
  m_quadMesh = GenMeshPlane(1.0f, 1.0f, 1, 1);
  m_material = LoadMaterialDefault();
  m_material.maps[MATERIAL_MAP_DIFFUSE].texture = effectTex;
  m_material.shader = instancedShader;

  m_viewLoc = GetShaderLocation(instancedShader, "view");
  m_billboardLoc =
      GetShaderLocation(instancedShader, "billboardMode"); // int uniform

  // CRITICAL: tell Raylib which attrib location receives per-instance matrices
  m_material.shader.locs[SHADER_LOC_MATRIX_MODEL] =
      GetShaderLocationAttrib(instancedShader, "instanceTransform");

  // Reserve a large buffer once — never reallocate during gameplay
  m_instanceBuffer.reserve(8192);
}

// ============================================================
// ParticleSystem :: AddEmitter
// ============================================================
std::shared_ptr<Emitter> ParticleSystem::AddEmitter(EmitterConfig config) {
  auto e = std::make_shared<Emitter>();
  e->Initialize(config);
  m_emitters.push_back(e);
  return e;
}

// ============================================================
// ParticleSystem :: RemoveEmitter
// ============================================================
void ParticleSystem::RemoveEmitter(std::shared_ptr<Emitter> emitter) {
  m_emitters.erase(std::remove(m_emitters.begin(), m_emitters.end(), emitter),
                   m_emitters.end());
}

// ============================================================
// ParticleSystem :: Update
// ============================================================
void ParticleSystem::Update(float deltaTime) {
  for (auto &e : m_emitters)
    e->Update(deltaTime);
}

void ParticleSystem::Draw(Camera3D cam) {
  m_instanceBuffer.clear();

  for (auto &e : m_emitters) {
    for (int k = 0; k < e->activeCount; k++) {
      int i = e->activeIndices[k];
      m_instanceBuffer.push_back(e->renderTransforms[i]);
    }
  }

  if (m_instanceBuffer.empty())
    return;

  Matrix viewMatrix = GetCameraMatrix(cam);
  SetShaderValueMatrix(m_material.shader, m_viewLoc, viewMatrix);

  // ==========================================
  // THE MISSING AAA FIXES
  // ==========================================
  rlDisableBackfaceCulling(); // Taaki peeche se bhi particles dikhein
  rlDisableDepthMask(); // Taaki particles ek doosre ke Z-buffer ko na kaatein
  BeginBlendMode(
      BLEND_ADDITIVE); // Alpha ki jagah Additive (Fire/Sparks glow effect!)

  DrawMeshInstanced(m_quadMesh, m_material, m_instanceBuffer.data(),
                    (int)m_instanceBuffer.size());

  EndBlendMode();
  rlEnableDepthMask();
  rlEnableBackfaceCulling();
  // ==========================================
}
