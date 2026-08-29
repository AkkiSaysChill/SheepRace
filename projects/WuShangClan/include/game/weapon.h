#pragma once

#include "engine/Engine.h"
#include "raylib.h"
#include <string>
#include <vector>

class Weapon {
public:
  Weapon(Engine *engine);
  ~Weapon();

  void Initialize();
  void Update(float deltaTime);
  void DrawHUD();
  void Shoot();

  int currentWeapon;
  int spamMode = 0;

  float m_fireRateTimer;
  float m_fireRate;

private:
  Engine *m_engine;

  Texture2D spamTex;
  Texture2D mijoTex;
  Texture2D bkgTex;
  float hudSize;

  std::vector<std::string> m_spamTexts;
  std::vector<std::string> m_rareTexts;

  struct ShockwaveVFX {
    Vector3 center;
    float currentRadius;
    float maxRadius;
    float speed;
    float alpha; // 1.0f (Solid)  0.0f (Invisible)
  };

  void SpawnTracer(Vector3 start, Vector3 end);

  std::vector<ShockwaveVFX> m_activeShockwaves;
};
