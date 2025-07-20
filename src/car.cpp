#include "car.h"
#include "raylib.h"
#include "raymath.h"

Car::Car() {
  model = LoadModel("resources/van.glb");
  scale = 1.0f;
  rotation = 0.0f;
  position = {0.0f, 0.0f, 0.0f};
  velocity = {0.0f, 0.0f, 0.0f};
  acceleration = {0.0f, 0.0f, 0.0f};
  speed = 0.0f;
  maxSpeed = 10.0f;
}

Car::~Car() { UnloadModel(model); }

void Car::Update(float deltaTime) {
  // Update acceleration based on input
  if (IsKeyDown(KEY_W)) {
    acceleration.z = -1.0f; // Move forward
  } else if (IsKeyDown(KEY_S)) {
    acceleration.z = 1.0f; // Move backward
  } else {
    acceleration.z = 0.0f; // Stop
  }

  // Update velocity and speed
  velocity.x += acceleration.x * deltaTime;
  velocity.y += acceleration.y * deltaTime;
  velocity.z += acceleration.z * deltaTime;

  speed = Vector3Length(velocity);

  // Limit speed
  if (speed > maxSpeed) {
    Vector3Normalize(velocity);
    velocity = Vector3Scale(velocity, maxSpeed);
  }

  // Update position
  position = Vector3Add(position, Vector3Scale(velocity, deltaTime));
}

void Car::Draw() {
  // Draw the car model
  DrawModel(model, position, scale, RAYWHITE);

  // Draw a simple representation of the car's direction
  Vector3 front =
      Vector3Add(position, Vector3Scale(Vector3Normalize(velocity), 1.0f));
  DrawLine3D(position, front, RED);

  // Optionally draw a rotation indicator
  DrawCircleV(Vector2{position.x, position.z}, 0.5f * scale, BLUE);
}
