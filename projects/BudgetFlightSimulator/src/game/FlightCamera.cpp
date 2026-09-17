#include "game/FlightCamera.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

FlightCamera::FlightCamera() {}

void FlightCamera::Initialize(float fov, float distance) {
  m_baseFov = fov;
  m_currentFov = fov;
  m_baseDistance = distance;
  m_distance = distance;

  m_camera.fovy = fov;
  m_camera.projection = CAMERA_PERSPECTIVE;
  m_camera.up = Vector3{0.0f, 1.0f, 0.0f};
  m_initialized = false;
}

void FlightCamera::AutoAdjustForModel(BoundingBox bounds) {
  float sizeX = bounds.max.x - bounds.min.x;
  float sizeY = bounds.max.y - bounds.min.y;
  float sizeZ = bounds.max.z - bounds.min.z;

  float maxDim = fmaxf(fmaxf(sizeX, sizeY), sizeZ);

  m_baseDistance = maxDim * 2.8f;
  m_distance = m_baseDistance;

  m_targetHeightOffset = fmaxf(1.8f, sizeY * 1.2f);
}

void FlightCamera::Update(const Aircraft &targetAircraft, float deltaTime) {
  if (deltaTime <= 0.0f)
    return;

  auto ExpSmooth = [](float current, float target, float lambda, float dt) {
    float t = 1.0f - expf(-lambda * dt);
    return current + (target - current) * t;
  };

  auto ExpSmoothVec3 = [](Vector3 current, Vector3 target, float lambda,
                          float dt) {
    float t = 1.0f - expf(-lambda * dt);
    return Vector3Lerp(current, target, t);
  };

  // 1. Mouse Free-Look Input Handling
  bool freeLookActive = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
                        IsMouseButtonDown(MOUSE_BUTTON_RIGHT);

  if (freeLookActive) {
    Vector2 mouseDelta = GetMouseDelta();
    const float mouseSensitivity = 0.0035f;

    m_freeLookYaw -= mouseDelta.x * mouseSensitivity;
    m_freeLookPitch += mouseDelta.y * mouseSensitivity;

    // Restrict pitch to avoid flipping upside down
    m_freeLookPitch = Clamp(m_freeLookPitch, -1.35f, 1.35f);
  } else {
    // Smoothly spring back to normal forward chase-cam
    m_freeLookYaw = ExpSmooth(m_freeLookYaw, 0.0f, 7.0f, deltaTime);
    m_freeLookPitch = ExpSmooth(m_freeLookPitch, 0.0f, 7.0f, deltaTime);
  }

  // 2. Speed and FOV Adjustments
  float airspeed = targetAircraft.GetAirspeedKmh();
  float targetSpeedFactor = Clamp((airspeed - 150.0f) / 650.0f, 0.0f, 1.0f);
  m_smoothedSpeedFactor =
      ExpSmooth(m_smoothedSpeedFactor, targetSpeedFactor, 3.5f, deltaTime);

  float targetFov = m_baseFov + m_smoothedSpeedFactor * 16.0f;
  float targetDistance = m_baseDistance + m_smoothedSpeedFactor * 4.0f;
  m_currentFov = ExpSmooth(m_currentFov, targetFov, 4.0f, deltaTime);

  Vector3 planePos = targetAircraft.position;
  Vector3 forward = Vector3Normalize(targetAircraft.GetForward());
  Vector3 aircraftUp = Vector3Normalize(targetAircraft.GetUp());
  Vector3 aircraftRight = Vector3Normalize(targetAircraft.GetRight());

  // 3. Direction Offsets in Local Space
  Vector3 desiredUp = Vector3Normalize(
      Vector3Add(Vector3{0.0f, 1.0f, 0.0f}, Vector3Scale(aircraftUp, 0.18f)));

  float lookAhead = 2.0f + m_smoothedSpeedFactor * 5.0f;
  Vector3 localLookOffset =
      Vector3Add(Vector3Scale(aircraftUp, m_targetHeightOffset),
                 Vector3Scale(forward, lookAhead));

  Vector3 localCamOffset =
      Vector3Subtract(Vector3Scale(aircraftUp, m_targetHeightOffset * 1.35f),
                      Vector3Scale(forward, targetDistance));

  // Apply Free-Look Spherical Orbiting to the Camera Offset
  if (fabsf(m_freeLookYaw) > 0.0001f || fabsf(m_freeLookPitch) > 0.0001f) {
    // Rotate offset around aircraft Up (Yaw) and Right (Pitch)
    Quaternion qYaw = QuaternionFromAxisAngle(aircraftUp, m_freeLookYaw);
    Quaternion qPitch = QuaternionFromAxisAngle(aircraftRight, m_freeLookPitch);
    Quaternion freeLookRot = QuaternionMultiply(qYaw, qPitch);

    localCamOffset = Vector3RotateByQuaternion(localCamOffset, freeLookRot);
  }

  // 4. Initialization Guard
  if (!m_initialized) {
    m_smoothPosition = localCamOffset;
    m_smoothTarget = localLookOffset;
    m_smoothUp = desiredUp;
    m_currentFov = targetFov;
    m_smoothedSpeedFactor = targetSpeedFactor;
    m_initialized = true;
  }

  // 5. Offset Smoothing
  // Faster responsiveness when actively looking around
  float posSmoothSpeed = freeLookActive ? 22.0f : 10.0f;
  m_smoothPosition = ExpSmoothVec3(m_smoothPosition, localCamOffset,
                                   posSmoothSpeed, deltaTime);
  m_smoothTarget =
      ExpSmoothVec3(m_smoothTarget, localLookOffset, 14.0f, deltaTime);
  m_smoothUp =
      Vector3Normalize(ExpSmoothVec3(m_smoothUp, desiredUp, 6.0f, deltaTime));

  // 6. Final World Alignment
  m_camera.position = Vector3Add(planePos, m_smoothPosition);
  m_camera.target = Vector3Add(planePos, m_smoothTarget);
  m_camera.up = m_smoothUp;
  m_camera.fovy = m_currentFov;
}
