#pragma once

#include "game/MissionData.h"
#include <unordered_map>
#include <vector>
#include <string>

class MissionDatabase {
public:
    static MissionDatabase& GetInstance();

    bool LoadFromMemory(const char* buffer, size_t size);
    bool LoadFromFile(const std::string& filePath);

    const MissionData* GetMission(const std::string& id) const;
    std::vector<std::string> GetAllMissionIds() const;

private:
    MissionDatabase() = default;
    StageType StringToStageType(const std::string& typeStr) const;
    EnemyType StringToEnemyType(const std::string& typeStr) const;

    std::unordered_map<std::string, MissionData> m_missions;
    std::vector<std::string> m_orderedIds;
};