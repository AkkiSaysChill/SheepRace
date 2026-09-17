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
  float m_smoothedSpeedFactor = 0.0f;
  // Distance & Framing
  float m_distance = 15.0f;
  float m_baseDistance = 15.0f;
  float m_minDistance = 6.0f;
  float m_maxDistance = 120.0f;
  float m_targetHeightOffset = 2.4f;

  float m_freeLookYaw = 0.0f;   // Radians
  float m_freeLookPitch = 0.0f; // Radians
  bool m_isFreeLooking = false;

  // Smoothed internal states (Prevents sub-frame physics vibration)
  Vector3 m_smoothPosition = {0.0f, 0.0f, 0.0f};
  Vector3 m_smoothTarget = {0.0f, 0.0f, 0.0f};
  Vector3 m_smoothUp = {0.0f, 1.0f, 0.0f};
  float m_baseFov = 55.0f;
  float m_currentFov = 55.0f;
  bool m_initialized = false;
};
