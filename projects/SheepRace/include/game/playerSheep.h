#pragma once

#include "BulletCollision/CollisionShapes/btShapeHull.h"
#include "engine/DrawableComponent.h"
#include "engine/GameObject.h"
#include "engine/PlayerProgress.h"
#include "engine/asset_loader.hpp"
#include "game/SheepPhysicsComponent.h"
#include "raylib.h"
#include <algorithm>
#include <vector>

class Car : public GameObject {
public:
  Car(PhysicsSystem *physicsSystem, AssetPacking::AssetLoader &assetLoader);
  ~Car();
  void Update(float deltaTime);

  Vector3 GetPosition() const { return position; }
  std::shared_ptr<SheepPhysicsComponent> getPhysicsComponent() const {
    return physicsComponent;
  }

  void setScale(float newScale);
  Model model;

  void AssignShader(Shader shader) {
    // (Make sure 'model' is loaded before calling this!)
    for (int i = 0; i < model.materialCount; i++) {
      model.materials[i].shader = shader;
    }
  }

  void DrawDebug();

private:
  std::shared_ptr<SheepPhysicsComponent> physicsComponent;
  btShapeHull *m_debugHull;

  // stats for the playerSheep
  Vector3 velocity;
  Vector3 acceleration;
  float speed;
  float maxSpeed;
  int sheepLevel = 0;
  float xp;

  std::vector<float> xp_threshold = {100.0f, 250.0f,  400.0f,
                                     750.0f, 1000.0f, 1500.0f};

  PlayerProgress m_progress;

  void calculateLevel();
};
