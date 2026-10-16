#pragma once
#include "game/MissionData.h"
#include <string>
#include <vector>

class FlightSession {
public:
  static FlightSession &GetInstance() {
    static FlightSession instance;
    return instance;
  }

  std::string activeMissionId = "";
  std::string selectedPlaneId = "";
  std::string sceneAfterComic = "";
  std::vector<ComicPanel> activeComicSequence;
};
