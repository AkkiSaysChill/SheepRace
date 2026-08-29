#pragma once
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"
#include "engine/DrawableComponent.h"
#include "engine/GameObject.h"
#include "engine/PhysicsComponent.h"
#include "game/playerSheep.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <iostream>
#include <random>
#include <vector>

enum b_state { active, idle, roam };

enum b_Nature { aggrasive, passive, dumb, smart, fast, slow };

struct Nature_Weight {
  b_Nature nature;
  int weight;
};

inline b_Nature getWeightNature() {
  static const std::vector<Nature_Weight> table = {{aggrasive, 50}, {fast, 30},
                                                   {smart, 5},      {dumb, 5},
                                                   {passive, 5},    {slow, 5}};

  int totalWeight = 0;
  for (const auto &item : table)
    totalWeight += item.weight;

  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, totalWeight - 1);

  static b_Nature lastPicked = slow;
  static int streakCount = 0;

  int randomValue = dis(gen);

  int currentSum = 0;
  for (const auto &item : table) {
    currentSum += item.weight;
    if (randomValue < currentSum) {
      return item.nature;
    }
  }

  b_Nature selection = aggrasive;

  if (selection == lastPicked) {
    streakCount++;
    // If we have seen this 2 times already, force a re-roll or pick the next
    // item
    if (streakCount >= 2) {
      // Force it to be 'Fast'
      selection = (b_Nature)((int)selection + 1);
      if (selection > slow)
        selection = aggrasive; // Wrap around
      streakCount = 0;         // Reset streak
    }
  } else {
    streakCount = 0;
  }

  lastPicked = selection;
  return selection;
}

class bulls : public GameObject // Inherit from GameObject
{
public:
  bulls(PhysicsSystem *physicsSystem, AssetPacking::AssetLoader &assetLoader,
        Car *playerPtr) {
    targetPlayer = playerPtr;
    nature = getWeightNature();
    ApplyNatureStats();

    // 1. Load Model
    this->model = assetLoader.LoadModelFromAssetData(
        assetLoader.getAssetData("bull.glb"), "bull.glb");

    this->position = {10.0f, 5.0f, 10.0f}; // Spawn position
    this->scale = {1.0f, 1.0f, 1.0f};

    btVector3 bullBoxHalfExtents(scale.x, scale.y, scale.z);
    btBoxShape *colShape = new btBoxShape(bullBoxHalfExtents);

    this->physicsComponent = AddComponent<PhysicsComponent>(
        physicsSystem, colShape, PhysicsShapeType::BOX, 1.0f, position,
        Quaternion{0, 0, 0, 1});

    AddComponent<DrawableComponent>(this->model);

    // Configure physics
    btRigidBody *body = physicsComponent->GetRigidBody();
    body->setDamping(0.2f, 0.2f);
    body->setActivationState(DISABLE_DEACTIVATION);
  }

  ~bulls() {
    if (m_debugHull)
      delete m_debugHull;
  }

  void AssignShader(Shader shader) {
    // (Make sure 'model' is loaded before calling this!)
    for (int i = 0; i < model.materialCount; i++) {
      model.materials[i].shader = shader;
    }
  }

  void Update(float deltaTime) {
    ChasePlayer();

    btTransform trans = physicsComponent->GetRigidBody()->getWorldTransform();
    btVector3 p = trans.getOrigin();
    this->position = {(float)p.x(), (float)p.y(), (float)p.z()};

    btQuaternion q = trans.getRotation();
    Quaternion physRot = {q.x(), q.y(), q.z(), q.w()};

    // Create a 90-degree corrective rotation on the X-axis (if it's "facing
    // down") If it's facing the wrong way horizontally, use DEG2RAD * 90 on the
    // Y axis
    // Quaternion correction = QuaternionFromEuler(90.0f * DEG2RAD, 0, 0);
    //
    // // Apply correction to the physics rotation
    // this->rotation = QuaternionMultiply(physRot, correction);
  }

  void ChasePlayer() {
    if (!targetPlayer || !physicsComponent)
      return;

    Vector3 p_pos = targetPlayer->GetPosition();
    Vector3 direction = Vector3Subtract(p_pos, this->position);
    direction.y = 0;

    float distance = Vector3Distance(p_pos, position);

    if (distance >= 5) {

      if (Vector3Length(direction) > 0.1f) {
        // Move
        Vector3 force = Vector3Scale(Vector3Normalize(direction), speed * 5.0f);
        btRigidBody *body = physicsComponent->GetRigidBody();
        body->applyCentralForce(btVector3(force.x, force.y, force.z));

        // Rotate Physics Body to face the player
        float angle = atan2f(direction.x, direction.z);
        btQuaternion targetRot(btVector3(0, 1, 0), angle);

        btTransform trans = body->getWorldTransform();
        trans.setRotation(targetRot);
        body->setWorldTransform(trans);
      }
    }
  }

  void DrawDebug() {
    if (!physicsComponent)
      return;

    btRigidBody *body = physicsComponent->GetRigidBody();
    btTransform trans = body->getWorldTransform();
    btVector3 pos = trans.getOrigin();
    btQuaternion rot = trans.getRotation();

    // Get the box dimensions from the shape
    btBoxShape *box = static_cast<btBoxShape *>(body->getCollisionShape());
    btVector3 halfExtents = box->getHalfExtentsWithMargin();

    Vector3 axis;
    float angle;
    Quaternion q = {(float)rot.x(), (float)rot.y(), (float)rot.z(),
                    (float)rot.w()};
    QuaternionToAxisAngle(q, &axis, &angle);

    rlPushMatrix();
    rlTranslatef((float)pos.x(), (float)pos.y(), (float)pos.z());
    rlRotatef(angle * RAD2DEG, axis.x, axis.y, axis.z);

    // Draw wireframe box using Raylib's rlgl
    Vector3 size = {halfExtents.x() * 2, halfExtents.y() * 2,
                    halfExtents.z() * 2};
    DrawCubeWiresV(Vector3Zero(), size, GREEN);

    rlPopMatrix();
  }

  Model model;

private:
  Car *targetPlayer;
  b_Nature nature;
  float speed;
  float turnSpeed;
  std::shared_ptr<PhysicsComponent> physicsComponent;
  btShapeHull *m_debugHull = nullptr;

  void ApplyNatureStats() {
    speed = 10.0f;
    turnSpeed = 2.0f;
    switch (nature) {
    case fast:
      speed = 15.0f;
      break;
    case slow:
      speed = 5.0f;
      break;
    case aggrasive:
      speed = 12.0f;
      turnSpeed = 3.0f;
      break;
    case dumb:
      turnSpeed = 0.5f;
      break;
    default:
      break;
    }
  }
};
