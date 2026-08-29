#pragma once
#include "json/json.hpp"
#include <iostream>
#include <string>
#include <vector>

struct LevelStat {
  bool unlocked = false;
  int stars = 0; // 0 to 3
  float bestTime = 0.0f;
};

class PlayerProgress {
public:
  // data to save
  int totalCurrency = 0;
  int currentLevelIndex = 0;
  int totalXP = 0;
  std::vector<LevelStat> levels;

  // Constructor: Initialize with some default levels
  PlayerProgress() {
    // Example: Create 5 empty levels, first one unlocked
    levels.resize(5);
    levels[0].unlocked = true;
  }

  void Save(const std::string &filename);
  bool Load(const std::string &filename);

  // Helper to add currency (e.g., when finishing a level)
  void AddCurrency(int amount) { totalCurrency += amount; }

  void AddXp(float xp_amount) { totalXP += xp_amount; }

  void CompleteLevel(int levelIndex, float time, int stars);
};
