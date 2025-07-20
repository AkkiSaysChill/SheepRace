#pragma once

#include "raylib.h"

class Car {
public:
  Car();
  ~Car();
  void Update(float deltaTime);
  void Draw();

  Model model;
  float scale;
  float rotation;
  Vector3 GetPosition() const { return position; }

  void setScale(float newScale) { scale = newScale; }

private:
  Vector3 position;
  Vector3 velocity;
  Vector3 acceleration;
  float speed;
  float maxSpeed;
};
