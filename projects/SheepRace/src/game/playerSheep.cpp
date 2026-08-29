#include "game/playerSheep.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "engine/PlayerProgress.h"
#include "game/SheepPhysicsComponent.h"
#include "iostream"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cstddef>

// TODO: have more sheep models
// add stats tu the sheeps

// TODO: add animations to the sheep
// fix the movement of the sheep

Car::Car(PhysicsSystem *physicsSystem, AssetPacking::AssetLoader &assetLoader) {
  assetLoader.loadPackedFile("cars.pack");
  this->model = assetLoader.LoadModelFromAssetData(
      assetLoader.getAssetData("sheep.glb"), "sheep.glb");
  m_progress.Load("player.save");

  // xp related stuff
  xp = m_progress.totalXP;
  calculateLevel();

  // player properties
  scale = {1.0f, 1.0f, 1.0f};
  position = {-5.0f, 3.0f, 0.0f};
  velocity = {0.0f, 0.0f, 0.0f};
  acceleration = {0.0f, 0.0f, 0.0f};
  speed = 0.0f;
  maxSpeed = 10.0f;

  // Create a collision shape
  btVector3 boxHalfExtents(1.0f, 0.5f, 1.0f);

  btBoxShape *colShape = new btBoxShape(boxHalfExtents);

  colShape->setLocalScaling(btVector3(scale.x, scale.y + 3.3f, scale.z + 1));

  m_debugHull = nullptr;

  physicsComponent = AddComponent<SheepPhysicsComponent>(
      physicsSystem, colShape, PhysicsShapeType::BOX, 0.5f, position,
      Quaternion{0, 0, 0, 1});

  // add drawable component
  AddComponent<DrawableComponent>(this->model);

  // Set damping and prevent deactivation
  btRigidBody *rigidBody = physicsComponent->GetRigidBody();
  rigidBody->setDamping(0.5f, 0.8f); // Linear and angular damping
  rigidBody->setActivationState(DISABLE_DEACTIVATION);
}

Car::~Car() {
  if (m_debugHull) {
    delete m_debugHull;
  }
}

void Car::calculateLevel() {
  for (size_t i = 0; i < xp_threshold.size(); i++) {
    if (xp > xp_threshold[i]) {
      sheepLevel += i;
    } else {
      break;
    }
  }
}

void Car::DrawDebug() {
  if (!physicsComponent || !m_debugHull)
    return;

  // 1. Get the current position/rotation from physics
  btRigidBody *body = physicsComponent->GetRigidBody();
  btTransform trans = body->getWorldTransform();

  btVector3 pos = trans.getOrigin();
  btQuaternion rot = trans.getRotation();

  // 2. Convert to Raylib rotation
  Vector3 axis;
  float angle;
  Quaternion q = {(float)rot.x(), (float)rot.y(), (float)rot.z(),
                  (float)rot.w()};
  QuaternionToAxisAngle(q, &axis, &angle);

  // 3. Begin Drawing
  rlPushMatrix();
  rlTranslatef((float)pos.x(), (float)pos.y(), (float)pos.z());
  rlRotatef(angle * RAD2DEG, axis.x, axis.y, axis.z);

  // Note: Scale is usually baked into the hull points by 'buildHull',
  // but if you dynamically resize the sheep, you might need rlScalef here.

  rlBegin(RL_LINES);
  rlColor4ub(0, 255, 0, 255); // Green color

  const btVector3 *vertices = m_debugHull->getVertexPointer();
  const unsigned int *indices = m_debugHull->getIndexPointer();
  int numTriangles = m_debugHull->numTriangles();

  for (int i = 0; i < numTriangles; i++) {
    // Get indices
    int i1 = indices[i * 3];
    int i2 = indices[i * 3 + 1];
    int i3 = indices[i * 3 + 2];

    // Get vertices
    btVector3 v1 = vertices[i1];
    btVector3 v2 = vertices[i2];
    btVector3 v3 = vertices[i3];

    // Draw Triangle Edges
    rlVertex3f(v1.x(), v1.y(), v1.z());
    rlVertex3f(v2.x(), v2.y(), v2.z());
    rlVertex3f(v2.x(), v2.y(), v2.z());
    rlVertex3f(v3.x(), v3.y(), v3.z());
    rlVertex3f(v3.x(), v3.y(), v3.z());
    rlVertex3f(v1.x(), v1.y(), v1.z());
  }

  rlEnd();
  rlPopMatrix();
}

void Car::Update(float deltaTime) {
  btRigidBody *rigidBody = physicsComponent->GetRigidBody();

  const float moveSpeed = -30.0f;
  const float turnSpeed = 4.0f;

  // --- Rotation (Torque) ---
  btVector3 angularVelocity(0, 0, 0);
  if (IsKeyDown(KEY_A)) {
    angularVelocity.setY(turnSpeed);
  }
  if (IsKeyDown(KEY_D)) {
    angularVelocity.setY(-turnSpeed);
  }
  rigidBody->setAngularVelocity(angularVelocity);

  // --- Forward/Backward Movement ---
  btTransform trans;
  rigidBody->getMotionState()->getWorldTransform(trans);

  // Update Position
  btVector3 btPos = trans.getOrigin();
  position = {(float)btPos.x(), (float)btPos.y(), (float)btPos.z()};

  // Update Rotation (Quaternion)
  btQuaternion btRot = trans.getRotation();
  rotation = {btRot.x(), btRot.y(), btRot.z(), btRot.w()};

  btVector3 forwardDirection = trans.getBasis().getColumn(2);

  btVector3 moveForce(0, 0, 0);
  if (IsKeyDown(KEY_W)) {
    moveForce = -forwardDirection * moveSpeed;
  }
  if (IsKeyDown(KEY_S)) {
    moveForce = forwardDirection * moveSpeed;
  }
  rigidBody->applyCentralForce(moveForce);

  // --- Jumping ---
  if (IsKeyPressed(KEY_SPACE)) { // Use IsKeyPressed for a single jump impulse
    // Check if the sheep is on the ground before jumping
    btVector3 velocity = rigidBody->getLinearVelocity();
    if (abs(velocity.y()) < 0.1) {
      rigidBody->applyCentralImpulse(btVector3(0, 10, 0));
    }
  }
}

void Car::setScale(float newScale) {
  scale = {newScale, newScale, newScale};

  if (physicsComponent) {
    btRigidBody *rigidBody = physicsComponent->GetRigidBody();
    if (rigidBody) {
      btCollisionShape *shape = rigidBody->getCollisionShape();
      shape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));

      // Re-calculate inertia for the new scale
      btScalar mass = rigidBody->getMass();
      btVector3 localInertia(0, 0, 0);
      if (mass != 0.0f) {
        shape->calculateLocalInertia(mass, localInertia);
      }
      rigidBody->setMassProps(mass, localInertia);
      rigidBody->updateInertiaTensor();
    }
  }
}
