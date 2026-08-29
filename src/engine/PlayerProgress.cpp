#include "engine/PlayerProgress.h"
#include <fstream>
#include <vector>

using json = nlohmann::json;

// SIMPLE ENCRYPTION KEY
// Changing this key will invalidate old save files!
static const uint8_t XOR_KEY = 0x5A; // 01011010 in binary

// Helper: XOR Scramble/Unscramble (Symmetric encryption)
void ScrambleData(std::vector<uint8_t>& data) {
    for (size_t i = 0; i < data.size(); i++) {
        data[i] ^= XOR_KEY;
    }
}

// Convert Struct to JSON -> BSON -> File
void PlayerProgress::Save(const std::string& filename) {
    json j;

    // Manual serialization (mapping variables to strings)
    j["currency"] = totalCurrency;
    j["xp"] = totalXP;
    j["currentLevel"] = currentLevelIndex;

    // Save Levels Array
    j["levels"] = json::array();
    for (const auto& lvl : levels) {
        j["levels"].push_back({
            {"unlocked", lvl.unlocked},
            {"stars", lvl.stars},
            {"time", lvl.bestTime}
            });
    }

    // "COMPILING" TO BINARY
    // Convert JSON to BSON (Binary Format)
    std::vector<uint8_t> binaryData = json::to_bson(j);

    // Obfuscate (Scramble) the data so it can't be opened in Notepad
    ScrambleData(binaryData);

    // Write raw bytes to file
    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<const char*>(binaryData.data()), binaryData.size());
    file.close();

    std::cout << "[SaveSystem] Progress compiled & saved to " << filename << std::endl;
}

// Read File -> Unscramble -> BSON -> JSON -> Struct
bool PlayerProgress::Load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate); // Open at end to check size
    if (!file.is_open()) {
        std::cout << "[SaveSystem] No save file found. Creating new profile." << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> binaryData(size);
    if (!file.read(reinterpret_cast<char*>(binaryData.data()), size)) {
        return false;
    }

    // Unscramble first
    ScrambleData(binaryData);

    try {
        // Parse BSON back to JSON
        json j = json::from_bson(binaryData);

        // Load into variables
        totalCurrency = j.value("currency", 0);
        totalXP = j.value("xp", 0);
        currentLevelIndex = j.value("currentLevel", 0);

        if (j.contains("levels")) {
            levels.clear();
            for (const auto& lvlJson : j["levels"]) {
                LevelStat stat;
                stat.unlocked = lvlJson.value("unlocked", false);
                stat.stars = lvlJson.value("stars", 0);
                stat.bestTime = lvlJson.value("time", 0.0f);
                levels.push_back(stat);
            }
        }
        std::cout << "[SaveSystem] Save loaded successfully!" << std::endl;
        return true;
    }
    catch (json::parse_error& e) {
        std::cerr << "[SaveSystem] Save file corrupted or wrong version!" << std::endl;
        return false;
    }
}

void PlayerProgress::CompleteLevel(int levelIndex, float time, int stars) {
    if (levelIndex < 0 || levelIndex >= levels.size()) return;

    // Update stats
    LevelStat& stat = levels[levelIndex];
    stat.unlocked = true;
    if (stars > stat.stars) stat.stars = stars;
    if (stat.bestTime == 0.0f || time < stat.bestTime) stat.bestTime = time;

    // Unlock next level
    if (levelIndex + 1 < levels.size()) {
        levels[levelIndex + 1].unlocked = true;
    }

    // Save immediately
    Save("player.save");
}