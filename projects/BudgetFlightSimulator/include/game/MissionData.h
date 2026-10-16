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
    DROP_PAYLOAD,     
    EVENT_TRIGGER,    
    TARGET_STRIKE,    
    LANDING,
    UNKNOWN
};

struct MissionStage {
    StageType type = StageType::UNKNOWN;
    std::string description;
    std::string eventAction; 
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    float radius = 60.0f;
    float minSpeed = 0.0f;    
    float maxSpeed = 9999.0f; 
    float minAltitude = 0.0f; 
};

struct ComicPanel {
    std::string imagePath;
    std::string audioPath;
    std::string subtitle;
};

struct MissionData {
    std::string id;
    std::string title;
    std::string description;
    std::string requiredPlaneId; 
    
    Vector3 spawnPosition = { -4850.0f, 111.5f, 150.0f };
    float spawnHeading = 90.0f; 
    float environmentTime = 12.0f;
    std::string environmentWeather = "CLEAR";
    bool hasRadar = false;
    std::vector<MissionStage> stages;
    std::vector<EnemyData> enemies;

    std::vector<ComicPanel> introComic;
    std::vector<ComicPanel> outroComic;
};
