#pragma once

#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "raylib.h"
enum class BossType {
  ITZARUGOD = 1,
  JAYIXH,
  SUNRAYBEE,
  BOA,
  ARPIT_BALA,
  DANK_RISHU
};

class Enemy : public GameObject {
public:
  Enemy(Engine *engine, BossType type, Vector3 spawnPos);
  ~Enemy() = default;

  void UpdateAI(Vector3 playerPos, float deltaTime);

  BossType m_type;
  float m_speed;

  Texture2D m_spriteTexture, m_spriteAttackTex, m_spriteAttack2Tex;
  Texture2D m_spriteWalk1Tex, m_spriteWalk2Tex;
  bool m_isSprite = false;
  Vector2 m_spriteSize;

private:
  Engine *m_engine;

  int m_state = 0;           // 0 = Walking, 1 = Attacking/Shooting
  float m_stateTimer = 0.0f; // Attack ka cooldown

  // Boss specific AI functions
  void UpdateAruAI(Vector3 playerPos, float deltaTime);
  void ShootAruLaser(Vector3 playerPos);
};
