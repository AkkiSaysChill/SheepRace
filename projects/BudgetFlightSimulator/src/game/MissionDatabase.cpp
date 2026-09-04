#include "game/MissionDatabase.h"
#include "game/tinyxml2.h"
#include <cstdio>
#include <vector>

MissionDatabase& MissionDatabase::GetInstance() {
    static MissionDatabase instance;
    return instance;
}

StageType MissionDatabase::StringToStageType(const std::string& typeStr) const {
    if (typeStr == "TAKEOFF")       return StageType::TAKEOFF;
    if (typeStr == "RING_GATE")     return StageType::RING_GATE;
    if (typeStr == "WAYPOINT")      return StageType::WAYPOINT;
    if (typeStr == "DROP_PAYLOAD")  return StageType::DROP_PAYLOAD;
    if (typeStr == "EVENT_TRIGGER") return StageType::EVENT_TRIGGER;
    if (typeStr == "TARGET_STRIKE") return StageType::TARGET_STRIKE;
    if (typeStr == "LANDING")       return StageType::LANDING;
    return StageType::UNKNOWN;
}

EnemyType MissionDatabase::StringToEnemyType(const std::string& typeStr) const {
    if (typeStr == "GROUND_RADAR")  return EnemyType::GROUND_RADAR;
    if (typeStr == "STATIC_TURRET") return EnemyType::STATIC_TURRET;
    return EnemyType::AIR_PATROL;
}

bool MissionDatabase::LoadFromMemory(const char* buffer, size_t size) {
    tinyxml2::XMLDocument doc;
    if (doc.Parse(buffer, size) != tinyxml2::XML_SUCCESS) return false;

    tinyxml2::XMLElement* root = doc.FirstChildElement("Missions");
    if (!root) return false;

    m_missions.clear();
    m_orderedIds.clear();

    for (tinyxml2::XMLElement* misElem = root->FirstChildElement("Mission"); 
         misElem != nullptr; 
         misElem = misElem->NextSiblingElement("Mission")) {

        MissionData mission;
        mission.id = misElem->Attribute("id") ? misElem->Attribute("id") : "unknown_mission";
        mission.title = misElem->Attribute("title") ? misElem->Attribute("title") : "Untitled Mission";
        mission.description = misElem->Attribute("description") ? misElem->Attribute("description") : "";
        mission.requiredPlaneId = misElem->Attribute("plane") ? misElem->Attribute("plane") : "";
        mission.environmentTime = 12.0f;
        mission.environmentWeather = "CLEAR";
        mission.spawnPosition = { -4850.0f, 111.5f, 150.0f };
        mission.spawnHeading = 90.0f;

        // 1. Parse <Spawn>
        tinyxml2::XMLElement* spawnElem = misElem->FirstChildElement("Spawn");
        if (spawnElem) {
            spawnElem->QueryFloatAttribute("x", &mission.spawnPosition.x);
            spawnElem->QueryFloatAttribute("y", &mission.spawnPosition.y);
            spawnElem->QueryFloatAttribute("z", &mission.spawnPosition.z);
            spawnElem->QueryFloatAttribute("heading", &mission.spawnHeading);
        }

        // 2. Parse <Environment>
        tinyxml2::XMLElement* envElem = misElem->FirstChildElement("Environment");
        if (envElem) {
            envElem->QueryFloatAttribute("time", &mission.environmentTime);
            if (envElem->Attribute("weather")) {
                mission.environmentWeather = envElem->Attribute("weather");
            }
        }

        // 3. Parse <Enemies>
        tinyxml2::XMLElement* enemiesNode = misElem->FirstChildElement("Enemies");
        if (enemiesNode) {
            for (tinyxml2::XMLElement* enmElem = enemiesNode->FirstChildElement("Enemy"); 
                 enmElem != nullptr; 
                 enmElem = enmElem->NextSiblingElement("Enemy")) {

                EnemyData enemy;
                enemy.id = enmElem->Attribute("id") ? enmElem->Attribute("id") : "enemy";
                enemy.name = enmElem->Attribute("name") ? enmElem->Attribute("name") : "Hostile";
                
                std::string typeStr = enmElem->Attribute("type") ? enmElem->Attribute("type") : "AIR_PATROL";
                enemy.type = StringToEnemyType(typeStr);
                enmElem->QueryIntAttribute("targetStage", &enemy.targetStageIndex);

                enemy.position = { 0.0f, 0.0f, 0.0f };
                tinyxml2::XMLElement* posElem = enmElem->FirstChildElement("Position");
                if (posElem) {
                    posElem->QueryFloatAttribute("x", &enemy.position.x);
                    posElem->QueryFloatAttribute("y", &enemy.position.y);
                    posElem->QueryFloatAttribute("z", &enemy.position.z);
                }

                enemy.health = 100.0f;
                tinyxml2::XMLElement* hpElem = enmElem->FirstChildElement("Health");
                if (hpElem) hpElem->QueryFloatText(&enemy.health);
                enemy.maxHealth = enemy.health;

                enemy.radius = 25.0f;
                tinyxml2::XMLElement* radElem = enmElem->FirstChildElement("Radius");
                if (radElem) radElem->QueryFloatText(&enemy.radius);

                enemy.speed = 0.0f;
                tinyxml2::XMLElement* spdElem = enmElem->FirstChildElement("Speed");
                if (spdElem) spdElem->QueryFloatText(&enemy.speed);

                enemy.isAlive = true;
                mission.enemies.push_back(enemy);
            }
        }

        // 4. Parse <Stage>
        for (tinyxml2::XMLElement* stgElem = misElem->FirstChildElement("Stage"); 
             stgElem != nullptr; 
             stgElem = stgElem->NextSiblingElement("Stage")) {

            MissionStage stage;
            stage.type = StringToStageType(stgElem->Attribute("type") ? stgElem->Attribute("type") : "");
            stage.description = stgElem->Attribute("description") ? stgElem->Attribute("description") : "";
            stage.eventAction = stgElem->Attribute("event") ? stgElem->Attribute("event") : "";
            stage.position = { 0.0f, 0.0f, 0.0f };
            stage.radius = 60.0f;
            stage.minSpeed = 0.0f;
            stage.maxSpeed = 9999.0f;
            stage.minAltitude = 0.0f;

            tinyxml2::XMLElement* posElem = stgElem->FirstChildElement("Position");
            if (posElem) {
                posElem->QueryFloatAttribute("x", &stage.position.x);
                posElem->QueryFloatAttribute("y", &stage.position.y);
                posElem->QueryFloatAttribute("z", &stage.position.z);
            }

            tinyxml2::XMLElement* radElem = stgElem->FirstChildElement("Radius");
            if (radElem) radElem->QueryFloatText(&stage.radius);

            tinyxml2::XMLElement* minSpdElem = stgElem->FirstChildElement("MinSpeed");
            if (minSpdElem) minSpdElem->QueryFloatText(&stage.minSpeed);

            tinyxml2::XMLElement* maxSpdElem = stgElem->FirstChildElement("MaxSpeed");
            if (maxSpdElem) maxSpdElem->QueryFloatText(&stage.maxSpeed);

            tinyxml2::XMLElement* minAltElem = stgElem->FirstChildElement("MinAltitude");
            if (minAltElem) minAltElem->QueryFloatText(&stage.minAltitude);

            mission.stages.push_back(stage);
        }

        m_missions[mission.id] = mission;
        m_orderedIds.push_back(mission.id);
    }

    return !m_missions.empty();
}

bool MissionDatabase::LoadFromFile(const std::string& filePath) {
    FILE* f = fopen(filePath.c_str(), "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<char> buf(sz);
    fread(buf.data(), 1, sz, f);
    fclose(f);

    return LoadFromMemory(buf.data(), buf.size());
}

const MissionData* MissionDatabase::GetMission(const std::string& id) const {
    auto it = m_missions.find(id);
    return (it != m_missions.end()) ? &it->second : nullptr;
}

std::vector<std::string> MissionDatabase::GetAllMissionIds() const {
    return m_orderedIds;
}