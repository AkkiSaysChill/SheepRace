#pragma once
#include "raylib.h"

struct ObjectSaveData {
  int objectType; // BossType ya PropType (e.g., 1 for Bed, 2 for Desk)
  Vector3 position;
  Vector3 rotation;
  Vector3 scale;
};
