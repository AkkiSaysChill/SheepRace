#pragma once

#include "game/MissionData.h"
#include "raymath.h"
#include <functional>
#include <string>
#include <vector>

class Engine;

using OnMissionSetupCallback = std::function<void(const std::string& planeId, Vector3 spawnPos, float headingDeg)>;
using OnMissionEventCallback = std::function<void(const std::string& eventName, const Vector3& stagePos)>;

class MissionManager {
public:
    MissionManager();

    void Initialize(Engine* engine);
    void SetSetupCallback(OnMissionSetupCallback cb) { m_onSetupCallback = cb; }
    void SetEventCallback(OnMissionEventCallback cb) { m_onEventCallback = cb; }

    bool StartMission(const std::string& missionId);
    void Update(float deltaTime, const Vector3& planePos, float airspeedKmh, float altitudeMeters, bool isOnGround, bool actionKeyPressed = false, bool triggerHit = false);
    
    void Draw3D(const Camera3D& camera);
    void DrawHUD(const Camera3D& camera);

    bool IsActive() const { return m_active; }
    bool IsCompleted() const { return m_completed; }
    const MissionData* GetCurrentMission() const { return m_currentMission; }
    int GetCurrentStageIndex() const { return m_currentStageIndex; }
    const MissionStage* GetCurrentStage() const;

private:
    void AdvanceStage();
    void ApplyMissionEnvironment();

    Engine* m_engine;
    const MissionData* m_currentMission;
    int m_currentStageIndex;
    bool m_active;
    bool m_completed;
    float m_completionBannerTimer;

    OnMissionSetupCallback m_onSetupCallback;
    OnMissionEventCallback m_onEventCallback;
};