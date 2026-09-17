#include "game/MissionManager.h"
#include "engine/Engine.h"
#include "game/MissionDatabase.h"
#include "rlgl.h"
#include <cmath>

MissionManager::MissionManager()
    : m_engine(nullptr), m_currentMission(nullptr), m_currentStageIndex(0),
      m_active(false), m_completed(false), m_missionTime(0.0f),
      m_completionBannerTimer(0.0f) {}

void MissionManager::Initialize(Engine *engine) { m_engine = engine; }

const MissionStage *MissionManager::GetCurrentStage() const {
  if (!m_active || !m_currentMission)
    return nullptr;
  if (m_currentStageIndex >= (int)m_currentMission->stages.size())
    return nullptr;
  return &m_currentMission->stages[m_currentStageIndex];
}

void MissionManager::ApplyMissionEnvironment() {
  if (!m_currentMission || !m_engine)
    return;

  m_engine->GetDayNightCycle().SetTime(m_currentMission->environmentTime);

  const std::string &w = m_currentMission->environmentWeather;
  WeatherPreset preset = WeatherPreset::CLEAR;
  if (w == "PARTLY_CLOUDY")
    preset = WeatherPreset::PARTLY_CLOUDY;
  else if (w == "OVERCAST")
    preset = WeatherPreset::OVERCAST;
  else if (w == "DENSE_FOG")
    preset = WeatherPreset::DENSE_FOG;
  else if (w == "STORMY_RAIN")
    preset = WeatherPreset::STORMY_RAIN;

  m_engine->GetWeatherSystem().SetWeather(preset, 1.0f);
}

bool MissionManager::StartMission(const std::string &missionId) {
  const MissionData *m = MissionDatabase::GetInstance().GetMission(missionId);
  if (!m || m->stages.empty())
    return false;

  m_currentMission = m;
  m_currentStageIndex = 0;
  m_active = true;
  m_completed = false;
  m_completionBannerTimer = 0.0f;
  m_missionTime = 0.0f;

  ApplyMissionEnvironment();

  if (m_onSetupCallback) {
    m_onSetupCallback(m->requiredPlaneId, m->spawnPosition, m->spawnHeading);
  }

  return true;
}

void MissionManager::AdvanceStage() {
  if (m_currentMission &&
      m_currentStageIndex < (int)m_currentMission->stages.size()) {
    const auto &stg = m_currentMission->stages[m_currentStageIndex];
    if (!stg.eventAction.empty() && m_onEventCallback) {
      m_onEventCallback(stg.eventAction, stg.position);
    }
  }

  m_currentStageIndex++;
  if (m_currentStageIndex >= (int)m_currentMission->stages.size()) {
    m_active = false;
    m_completed = true;
    m_completionBannerTimer = 5.0f;
  }
}

void MissionManager::Update(float deltaTime, const Vector3 &planePos,
                            float airspeedKmh, float altitudeMeters,
                            bool isOnGround, bool actionKeyPressed,
                            bool triggerHit) {
  if (m_completionBannerTimer > 0.0f) {
    m_completionBannerTimer -= deltaTime;
  }

  if (!m_active || !m_currentMission)
    return;
  m_missionTime += deltaTime;

  if (m_currentStageIndex >= (int)m_currentMission->stages.size())
    return;

  const MissionStage &stage = m_currentMission->stages[m_currentStageIndex];
  float dist = Vector3Distance(planePos, stage.position);

  switch (stage.type) {
  case StageType::TAKEOFF:
    if (altitudeMeters >= stage.minAltitude && airspeedKmh >= stage.minSpeed &&
        !isOnGround) {
      AdvanceStage();
    }
    break;

  case StageType::RING_GATE:
  case StageType::WAYPOINT:
    if (dist <= stage.radius) {
      AdvanceStage();
    }
    break;

  case StageType::DROP_PAYLOAD: {
    float dx = planePos.x - stage.position.x;
    float dz = planePos.z - stage.position.z;
    float horizDist = sqrtf(dx * dx + dz * dz);

    // Trigger when within horizontal radius and G key is pressed
    if (horizDist <= stage.radius && actionKeyPressed) {
      AdvanceStage();
    }
    break;
  }

  case StageType::EVENT_TRIGGER:
    if (dist <= stage.radius) {
      AdvanceStage();
    }
    break;

  case StageType::TARGET_STRIKE:
    // Destroyed by cannon/missile or direct precision hit
    if (triggerHit || dist <= (stage.radius * 0.45f)) {
      AdvanceStage();
    }
    break;

  case StageType::LANDING:
    if (dist <= stage.radius && isOnGround && airspeedKmh <= stage.maxSpeed) {
      AdvanceStage();
    }
    break;

  default:
    break;
  }
}

void MissionManager::Draw3D(const Camera3D &camera) {
  if (!m_active || !m_currentMission)
    return;
  if (m_currentStageIndex >= (int)m_currentMission->stages.size())
    return;

  const MissionStage &stage = m_currentMission->stages[m_currentStageIndex];

  rlDisableDepthMask();
  rlDisableBackfaceCulling();

  if (stage.type == StageType::RING_GATE) {
    Vector3 forward =
        Vector3Normalize(Vector3Subtract(camera.position, stage.position));
    Vector3 up = {0.0f, 1.0f, 0.0f};
    Vector3 right = Vector3CrossProduct(forward, up);
    up = Vector3CrossProduct(right, forward);

    DrawCircle3D(stage.position, stage.radius, forward, 0.0f,
                 Fade(LIME, 0.25f));

    int segments = 24;
    for (int i = 0; i < segments; i++) {
      float a1 = ((float)i / segments) * 2.0f * PI;
      float a2 = ((float)(i + 1) / segments) * 2.0f * PI;
      Vector3 p1 =
          Vector3Add(stage.position,
                     Vector3Add(Vector3Scale(right, cosf(a1) * stage.radius),
                                Vector3Scale(up, sinf(a1) * stage.radius)));
      Vector3 p2 =
          Vector3Add(stage.position,
                     Vector3Add(Vector3Scale(right, cosf(a2) * stage.radius),
                                Vector3Scale(up, sinf(a2) * stage.radius)));
      DrawLine3D(p1, p2, GREEN);
    }
  } else if (stage.type == StageType::DROP_PAYLOAD) {
    DrawCircle3D(stage.position, stage.radius, {0, 1, 0}, 0.0f,
                 Fade(RED, 0.25f));
    DrawCylinderWires(stage.position, stage.radius, stage.radius, 120.0f, 16,
                      ORANGE);
  } else if (stage.type == StageType::TARGET_STRIKE) {
    // Red combat diamond
    DrawSphereWires(stage.position, 12.0f, 8, 8, RED);
    DrawCircle3D(stage.position, stage.radius, {0, 1, 0}, 0.0f,
                 Fade(RED, 0.15f));
  } else if (stage.type == StageType::WAYPOINT ||
             stage.type == StageType::TAKEOFF ||
             stage.type == StageType::LANDING ||
             stage.type == StageType::EVENT_TRIGGER) {
    DrawCircle3D(stage.position, stage.radius, {0, 1, 0}, 0.0f,
                 Fade(YELLOW, 0.2f));
    DrawCylinderWires(stage.position, stage.radius, stage.radius, 40.0f, 16,
                      GOLD);
  }

  rlEnableBackfaceCulling();
  rlEnableDepthMask();
}

void MissionManager::DrawHUD(const Camera3D &camera) {
  int screenW = GetScreenWidth();

  if (m_completionBannerTimer > 0.0f) {
    DrawRectangle(screenW / 2 - 220, 110, 440, 50, Fade(BLACK, 0.75f));
    DrawRectangleLines(screenW / 2 - 220, 110, 440, 50, GOLD);
    DrawText("MISSION ACCOMPLISHED!", screenW / 2 - 165, 122, 24, GOLD);
  }

  if (!m_active || !m_currentMission)
    return;
  if (m_currentStageIndex >= (int)m_currentMission->stages.size())
    return;

  const MissionStage &stage = m_currentMission->stages[m_currentStageIndex];

  DrawRectangle(screenW / 2 - 250, 20, 500, 70, Fade(BLACK, 0.65f));
  DrawRectangleLines(screenW / 2 - 250, 20, 500, 70, SKYBLUE);

  DrawText(TextFormat("MISSION: %s (%d/%d)", m_currentMission->title.c_str(),
                      m_currentStageIndex + 1,
                      (int)m_currentMission->stages.size()),
           screenW / 2 - 235, 30, 18, YELLOW);

  if (stage.type == StageType::DROP_PAYLOAD) {
    DrawText(TextFormat("OBJECTIVE: %s [PRESS G TO DROP]",
                        stage.description.c_str()),
             screenW / 2 - 235, 54, 16, ORANGE);
  } else if (stage.type == StageType::TARGET_STRIKE) {
    DrawText(
        TextFormat("COMBAT TARGET: %s [FIRE GUNS]", stage.description.c_str()),
        screenW / 2 - 235, 54, 16, RED);
  } else {
    DrawText(TextFormat("OBJECTIVE: %s", stage.description.c_str()),
             screenW / 2 - 235, 54, 16, RAYWHITE);
  }

  Vector2 screenPos = GetWorldToScreen(stage.position, camera);
  Vector3 toTarget = Vector3Subtract(stage.position, camera.position);
  Vector3 camFwd =
      Vector3Normalize(Vector3Subtract(camera.target, camera.position));

  if (Vector3DotProduct(toTarget, camFwd) > 0.0f) {
    Color markerColor = LIME;
    if (stage.type == StageType::DROP_PAYLOAD)
      markerColor = ORANGE;
    else if (stage.type == StageType::TARGET_STRIKE)
      markerColor = RED;

    DrawRectangleLines((int)screenPos.x - 15, (int)screenPos.y - 15, 30, 30,
                       markerColor);
    float distKm = Vector3Length(toTarget) / 1000.0f;
    DrawText(TextFormat("%.1f km", distKm), (int)screenPos.x - 22,
             (int)screenPos.y + 18, 14, markerColor);
  }
}
