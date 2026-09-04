#pragma once

#include "raylib.h"
#include "raymath.h"

struct LightingState {
    Vector3 lightDir;       // Direction TO the key light
    Vector4 lightColor;     // Sunlight / moonlight color
    Vector4 ambientColor;   // Tri-ambient base tone
    Vector3 skyColor;       // Sky zenith color
    Vector3 equatorColor;   // Horizon transition color
    Vector3 groundColor;    // Ground ambient reflection
    float   cloudCover;     // Cloud density
    float   cloudFalloff;   // Cloud edge contrast
    bool    isNight;        // True if moon is dominant light
};

class DayNightCycle {
public:
    DayNightCycle();

    void SetTime(float hours);
    float GetTime() const { return m_timeOfDay; }

    void SetTimeScale(float scale) { m_timeScale = scale; }
    float GetTimeScale() const { return m_timeScale; }

    void SetPaused(bool paused) { m_isPaused = paused; }
    bool IsPaused() const { return m_isPaused; }

    void Update(float deltaTime);
    void ApplyToShaders(Shader shadowShader, Shader skyShader);

    const LightingState& GetCurrentLighting() const { return m_currentLighting; }

private:
    void CalculateLighting();

    float m_timeOfDay;
    float m_timeScale;
    bool  m_isPaused;

    LightingState m_currentLighting;
};