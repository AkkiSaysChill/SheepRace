#include "game/FlightCamera.h"
#include <cmath>

FlightCamera::FlightCamera() {
  m_camera.up = {0.0f, 1.0f, 0.0f};
  m_camera.fovy = 55.0f;
  m_camera.projection = CAMERA_PERSPECTIVE;
}

void FlightCamera::Initialize(float fov, float distance) {
  m_camera.fovy = fov;
  m_distance = distance;
  m_baseDistance = distance;
  m_camera.up = {0.0f, 1.0f, 0.0f};
  m_camera.projection = CAMERA_PERSPECTIVE;
}

void FlightCamera::AutoAdjustForModel(BoundingBox bounds) {
  // Calculate model extents along X, Y, Z
  float width = fabsf(bounds.max.x - bounds.min.x);
  float height = fabsf(bounds.max.y - bounds.min.y);
  float length = fabsf(bounds.max.z - bounds.min.z);

  // Find the largest dimension (wingspan or fuselage length)
  float maxDim = fmaxf(width, fmaxf(height, length));
  if (maxDim < 1.0f)
    maxDim = 4.0f; // Safe fallback

  // Scale chase distance and camera focus height proportionally
  m_baseDistance = maxDim * 2.2f;
  m_distance = m_baseDistance;

  m_minDistance = maxDim * 0.8f;
  m_maxDistance = maxDim * 5.0f;
  m_targetHeightOffset = fmaxf(height * 0.5f, 0.8f);
}

void FlightCamera::Update(const Aircraft &targetAircraft, float deltaTime) {
  // 1. Zoom via mouse scroll
  float wheel = GetMouseWheelMove();
  if (wheel != 0.0f) {
    m_distance -= wheel * (m_baseDistance * 0.1f);
    m_distance = Clamp(m_distance, m_minDistance, m_maxDistance);
  }

  // 2. Free-look Orbit Drag
  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
      IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    Vector2 mousePos = GetMousePosition();
    if (m_isOrbiting) {
      Vector2 delta = Vector2Subtract(mousePos, m_prevMousePos);
      m_yaw -= delta.x * 0.25f;
      m_pitch += delta.y * 0.25f;
      m_pitch = Clamp(m_pitch, -80.0f, 85.0f);
    }
    m_prevMousePos = mousePos;
    m_isOrbiting = true;
  } else {
    m_isOrbiting = false;
    m_yaw = Lerp(m_yaw, 0.0f, 2.5f * deltaTime);
    m_pitch = Lerp(m_pitch, 12.0f, 2.5f * deltaTime);
  }

  // 3. Compute relative spherical offset based on aircraft coordinate frame
  Vector3 forward = targetAircraft.GetForward();
  Vector3 up = targetAircraft.GetUp();
  Vector3 right = targetAircraft.GetRight();

  float pitchRad = m_pitch * DEG2RAD;
  float yawRad = m_yaw * DEG2RAD;

  Vector3 localDir = {sinf(yawRad) * cosf(pitchRad), sinf(pitchRad),
                      -cosf(yawRad) * cosf(pitchRad)};

  Vector3 worldOffset =
      Vector3Add(Vector3Add(Vector3Scale(right, localDir.x * m_distance),
                            Vector3Scale(up, localDir.y * m_distance)),
                 Vector3Scale(forward, localDir.z * m_distance));

  m_camera.position = Vector3Add(targetAircraft.position, worldOffset);
  m_camera.target = Vector3Add(targetAircraft.position,
                               Vector3Scale(up, m_targetHeightOffset));
}
