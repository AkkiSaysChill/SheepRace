#pragma once
#include "raylib.h"
#include "raymath.h"
#include <string>

struct FlightInput {
    float pitch = 0.0f;
    float roll = 0.0f;
    float yaw = 0.0f;
    float throttle = 0.0f;
    bool brake = false;
};

struct AircraftStats {
    std::string id = "trainer";
    std::string name = "Default Trainer";
    std::string modelFile = "plane.glb";

    float maxThrustSpeed = 95.0f;
    float stallSpeedThreshold = 22.0f;
    float throttleResponse = 8.0f;
    float brakeDecel = 14.0f;
    float baseDragFactor = 1.2f;
    float climbDragMultiplier = 38.0f;
    float glideSinkRate = 1.2f;
    float bankSinkStrength = 3.2f;
    float pitchRate = 1.05f;
    float rollRate = 1.10f;
    float yawRate = 0.55f;
    float pitchInertia = 1.8f;
    float bankNoseSlip = 0.35f;
};

class Aircraft {
public:
    Aircraft();

    AircraftStats stats;

    Vector3 position = { 0.0f, 150.0f, 0.0f };
    Quaternion orientation = QuaternionIdentity();
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };
    Vector3 angularVelocity = { 0.0f, 0.0f, 0.0f };

    bool isStalling = false;
    bool isOnGround = false;
    float currentGForce = 1.0f;

    void SetStats(const AircraftStats& newStats);
    void Update(const FlightInput& input, float dt);
    void Reset(Vector3 spawnPos);

    Vector3 GetForward() const;
    Vector3 GetUp() const;
    Vector3 GetRight() const;

    float GetAirspeedKmh() const;
    float GetAirspeedKnots() const;
    float GetAltitudeMeters() const;
    float GetVerticalSpeed() const;
    float GetPitchAngle() const;
    float GetRollAngle() const;
    float GetHeading() const;
};