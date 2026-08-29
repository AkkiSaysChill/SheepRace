#pragma once
#include <string>

class FlightSession {
public:
  static FlightSession &GetInstance() {
    static FlightSession instance;
    return instance;
  }

  std::string selectedPlaneId = "trainer";

private:
  FlightSession() = default;
};
