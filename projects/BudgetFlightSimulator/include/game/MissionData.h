#pragma once

#include "raylib.h"
#include <string>
#include <vector>

enum class EnemyType {
    AIR_PATROL,
    GROUND_RADAR,
    STATIC_TURRET
};

struct EnemyData {
    std::string id;
    std::string name;
    EnemyType type = EnemyType::AIR_PATROL;
    int targetStageIndex = -1; // Associated TARGET_STRIKE stage index
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    float health = 100.0f;
    float maxHealth = 100.0f;
    float radius = 20.0f;
    float speed = 0.0f;
    bool isAlive = true;
};

enum class StageType {
    TAKEOFF,
    RING_GATE,
    WAYPOINT,
    DROP_PAYLOAD,     // Drop emergency cargo / medical kits (action key [G])
    EVENT_TRIGGER,    // Scripted events (ENGINE_FAIL, etc.)
    TARGET_STRIKE,    // Lock-on and destroy drone / enemy ace
    LANDING,
    UNKNOWN
};

struct MissionStage {
    StageType type = StageType::UNKNOWN;
    std::string description;
    std::string eventAction; // e.g. "DROP_CARGO", "ENGINE_FAIL", "DESTROY_TARGET"
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    float radius = 60.0f;
    float minSpeed = 0.0f;    // (km/h)
    float maxSpeed = 9999.0f; // (km/h)
    float minAltitude = 0.0f; // (m)
};

struct MissionData {
    std::string id;
    std::string title;
    std::string description;
    std::string requiredPlaneId; // "trainer", "jet_mk2", "cargo", etc.
    
    Vector3 spawnPosition = { -4850.0f, 111.5f, 150.0f };
    float spawnHeading = 90.0f; // Degrees (0 = North, 90 = East)
    float environmentTime = 12.0f;
    std::string environmentWeather = "CLEAR";
    
    std::vector<MissionStage> stages;
    std::vector<EnemyData> enemies;
};