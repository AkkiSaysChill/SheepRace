#pragma once

#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>

enum class WeatherPreset {
    CLEAR = 0,
    PARTLY_CLOUDY,
    OVERCAST,
    DENSE_FOG,
    STORMY_RAIN,
    COUNT
};

struct WeatherProfile {
    float cloudCover;        // 0.0 (clear) to 1.0 (thick overcast)
    float cloudFalloff;      // Softness / edge contrast
    float sunDimFactor;      // Multiplier on sunlight/moonlight (darkens during storms)
    float fogDensity;        // Distance fog thickness
    Vector3 horizonHazeTint; // Sky/horizon blend tint
    Vector3 windVector;      // In-world wind velocity in m/s (X, Y, Z)
    float turbulence;        // Shake/jitter factor for flight controls
    float rainIntensity;     // 0.0 (dry) to 1.0 (torrential downpour)
};

struct RainParticle {
    Vector3 offset;          // Position relative to camera box
    float speed;             // Fall rate
    float length;            // Streak segment length
};

class WeatherSystem {
public:
    WeatherSystem();

    void Initialize(int rainDropCount = 1200);
    void Update(float deltaTime, const Vector3& cameraPos, const Vector3& aircraftVel);
    void Draw3D(const Vector3& cameraPos);

    void SetWeather(WeatherPreset preset, float transitionDuration = 4.0f);
    void CycleNextWeather(float transitionDuration = 4.0f);

    WeatherPreset GetCurrentPreset() const { return m_currentPreset; }
    std::string GetCurrentWeatherName() const;
    const WeatherProfile& GetActiveProfile() const { return m_activeProfile; }

    bool IsTransitioning() const { return m_transitionProgress < 1.0f; }
    float GetTransitionProgress() const { return m_transitionProgress; }

    void SetAutoWeatherEnabled(bool enabled) { m_autoWeatherEnabled = enabled; }
    bool IsAutoWeatherEnabled() const { return m_autoWeatherEnabled; }

    // Set how long a weather state lasts in real seconds (min, max)
    void SetWeatherDurationRange(float minSeconds, float maxSeconds) {
        m_minDuration = minSeconds;
        m_maxDuration = maxSeconds;
    }

    void ResetStateTimer(); // <-- Publicly accessible for manual overrides

private:
    WeatherProfile GetPresetProfile(WeatherPreset preset) const;
    void BlendProfiles(const WeatherProfile& a, const WeatherProfile& b, float t);

    WeatherPreset m_currentPreset;
    WeatherPreset m_targetPreset;
    float m_transitionDuration;
    float m_transitionProgress;

    WeatherProfile m_currentProfile;
    WeatherProfile m_activeProfile;

    // Local Camera-Pinned Rain Volume
    std::vector<RainParticle> m_rainDrops;
    float m_rainBoxSize;

    WeatherPreset PickNextWeatherPreset(WeatherPreset current) const;

    bool  m_autoWeatherEnabled = true;
    float m_stateTimer = 0.0f;
    float m_stateDuration = 180.0f;   // Current weather duration target
    float m_minDuration = 120.0f;     // Minimum stay in one state (2 min)
    float m_maxDuration = 300.0f;     // Maximum stay in one state (5 min)
};