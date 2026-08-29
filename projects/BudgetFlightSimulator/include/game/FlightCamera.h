#pragma once
#include "game/Aircraft.h"
#include "raylib.h"
#include "raymath.h"

class FlightCamera {
public:
  FlightCamera();

  void Initialize(float fov = 55.0f, float distance = 14.0f);
  void Update(const Aircraft &targetAircraft, float deltaTime);

  // Auto-fit framing based on aircraft 3D model dimensions
  void AutoAdjustForModel(BoundingBox bounds);

  Camera3D GetRaylibCamera() const { return m_camera; }
  void BeginMode() const { BeginMode3D(m_camera); }
  void EndMode() const { EndMode3D(); }

  void SetDistance(float dist) {
    m_distance = Clamp(dist, m_minDistance, m_maxDistance);
  }
  float GetDistance() const { return m_distance; }

private:
  Camera3D m_camera = {0};

  float m_distance = 14.0f;
  float m_baseDistance = 14.0f;
  float m_minDistance = 5.0f;
  float m_maxDistance = 120.0f;
  float m_targetHeightOffset = 1.2f;

  float m_pitch = 12.0f;
  float m_yaw = 0.0f;
  Vector2 m_prevMousePos = {0.0f, 0.0f};
  bool m_isOrbiting = false;
};
