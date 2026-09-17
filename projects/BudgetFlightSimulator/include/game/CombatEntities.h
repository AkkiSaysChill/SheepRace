#pragma once

#include "raylib.h"
#include "raymath.h"
#include <string>

struct BulletTracer {
  Vector3 position;
  Vector3 velocity;
  float life;
  bool isEnemy;
  bool active;
};

struct HomingMissile {
  Vector3 position;
  Vector3 velocity;
  Vector3 targetPos;
  bool isTrackingPlayer;
  float speed;
  float turnRate; // Radians per second
  float life;
  float blastRadius;
  float damage;
  bool active;
};

struct EnemyAce {
  std::string id;
  std::string name;
  Vector3 position;
  Quaternion orientation;
  Vector3 velocity;
  float health;
  float maxHealth;
  float hitRadius;
  bool isAlive;
  float evadeTimer = 0.0f;
  // AI Weapon cooldowns
  float gunCooldown;
  float missileCooldown;
};
