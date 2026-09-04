#include "engine/WeatherSystem.h"
#include <cstdlib>
#include <algorithm>
#include "rlgl.h"

#include <cstdlib>

// Helper to pick a random float between [min, max]
static float RandomFloat(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

void WeatherSystem::ResetStateTimer() {
    m_stateTimer = 0.0f;
    m_stateDuration = RandomFloat(m_minDuration, m_maxDuration);
}

WeatherPreset WeatherSystem::PickNextWeatherPreset(WeatherPreset current) const {
    float roll = RandomFloat(0.0f, 1.0f);

    switch (current) {
        case WeatherPreset::CLEAR:
            // 75% Partly Cloudy, 25% Fog
            return (roll < 0.75f) ? WeatherPreset::PARTLY_CLOUDY : WeatherPreset::DENSE_FOG;

        case WeatherPreset::PARTLY_CLOUDY:
            // 45% Overcast, 45% Clear, 10% Fog
            if (roll < 0.45f) return WeatherPreset::OVERCAST;
            if (roll < 0.90f) return WeatherPreset::CLEAR;
            return WeatherPreset::DENSE_FOG;

        case WeatherPreset::OVERCAST:
            // 40% Stormy Rain, 35% Partly Cloudy, 25% Fog
            if (roll < 0.40f) return WeatherPreset::STORMY_RAIN;
            if (roll < 0.75f) return WeatherPreset::PARTLY_CLOUDY;
            return WeatherPreset::DENSE_FOG;

        case WeatherPreset::DENSE_FOG:
            // 40% Overcast, 40% Partly Cloudy, 20% Clear
            if (roll < 0.40f) return WeatherPreset::OVERCAST;
            if (roll < 0.80f) return WeatherPreset::PARTLY_CLOUDY;
            return WeatherPreset::CLEAR;

        case WeatherPreset::STORMY_RAIN:
            // Storms always subside into Overcast (80%) or lingering Fog (20%)
            return (roll < 0.80f) ? WeatherPreset::OVERCAST : WeatherPreset::DENSE_FOG;

        default:
            return WeatherPreset::CLEAR;
    }
}

static Vector3 Vector3LerpRGB(Vector3 a, Vector3 b, float t) {
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

WeatherSystem::WeatherSystem()
    : m_currentPreset(WeatherPreset::PARTLY_CLOUDY),
      m_targetPreset(WeatherPreset::PARTLY_CLOUDY),
      m_transitionDuration(1.0f),
      m_transitionProgress(1.0f),
      m_rainBoxSize(80.0f) {
    m_currentProfile = GetPresetProfile(m_currentPreset);
    m_activeProfile = m_currentProfile;
}

WeatherProfile WeatherSystem::GetPresetProfile(WeatherPreset preset) const {
    WeatherProfile p;
    switch (preset) {
        case WeatherPreset::CLEAR:
            p.cloudCover       = 0.12f;
            p.cloudFalloff     = 1.10f;
            p.sunDimFactor     = 1.00f;
            p.fogDensity       = 0.00008f;
            p.horizonHazeTint  = { 0.60f, 0.75f, 0.95f };
            p.windVector       = { 2.0f, 0.0f, 1.0f };
            p.turbulence       = 0.02f;
            p.rainIntensity    = 0.0f;
            break;

        case WeatherPreset::PARTLY_CLOUDY:
            p.cloudCover       = 0.40f;
            p.cloudFalloff     = 0.95f;
            p.sunDimFactor     = 0.95f;
            p.fogDensity       = 0.00012f;
            p.horizonHazeTint  = { 0.58f, 0.72f, 0.88f };
            p.windVector       = { 5.0f, 0.0f, -3.0f };
            p.turbulence       = 0.08f;
            p.rainIntensity    = 0.0f;
            break;

        case WeatherPreset::OVERCAST:
            p.cloudCover       = 0.82f;
            p.cloudFalloff     = 0.65f;
            p.sunDimFactor     = 0.50f;
            p.fogDensity       = 0.00028f;
            p.horizonHazeTint  = { 0.42f, 0.45f, 0.52f };
            p.windVector       = { 12.0f, 0.0f, 8.0f };
            p.turbulence       = 0.22f;
            p.rainIntensity    = 0.0f;
            break;

        case WeatherPreset::DENSE_FOG:
            p.cloudCover       = 0.70f;
            p.cloudFalloff     = 0.45f;
            p.sunDimFactor     = 0.35f;
            p.fogDensity       = 0.00095f;
            p.horizonHazeTint  = { 0.50f, 0.52f, 0.56f };
            p.windVector       = { 1.5f, 0.0f, 1.0f };
            p.turbulence       = 0.04f;
            p.rainIntensity    = 0.10f; // Slight mist
            break;

        case WeatherPreset::STORMY_RAIN:
            p.cloudCover       = 0.95f;
            p.cloudFalloff     = 0.40f;
            p.sunDimFactor     = 0.22f; // Very dark stormy day
            p.fogDensity       = 0.00055f;
            p.horizonHazeTint  = { 0.28f, 0.32f, 0.38f };
            p.windVector       = { 22.0f, -3.0f, 16.0f }; // Strong wind
            p.turbulence       = 0.65f; // Heavy turbulence
            p.rainIntensity    = 1.0f;  // Torrential downpour
            break;

        default:
            break;
    }
    return p;
}

void WeatherSystem::Initialize(int rainDropCount) {
    m_rainDrops.resize(rainDropCount);
    float half = m_rainBoxSize * 0.5f;

    for (auto& drop : m_rainDrops) {
        drop.offset = {
            ((float)rand() / (float)RAND_MAX) * m_rainBoxSize - half,
            ((float)rand() / (float)RAND_MAX) * m_rainBoxSize - half,
            ((float)rand() / (float)RAND_MAX) * m_rainBoxSize - half
        };
        drop.speed = 45.0f + ((float)rand() / (float)RAND_MAX) * 35.0f;
        drop.length = 1.2f + ((float)rand() / (float)RAND_MAX) * 1.6f;
    }
}

void WeatherSystem::SetWeather(WeatherPreset preset, float transitionDuration) {
    if (preset == m_currentPreset && m_transitionProgress >= 1.0f) return;
    m_currentProfile = m_activeProfile;
    m_targetPreset = preset;
    m_transitionDuration = std::max(transitionDuration, 0.05f);
    m_transitionProgress = 0.0f;
}

void WeatherSystem::CycleNextWeather(float transitionDuration) {
    int next = ((int)m_targetPreset + 1) % (int)WeatherPreset::COUNT;
    SetWeather((WeatherPreset)next, transitionDuration);
}

void WeatherSystem::BlendProfiles(const WeatherProfile& a, const WeatherProfile& b, float t) {
    m_activeProfile.cloudCover      = a.cloudCover + (b.cloudCover - a.cloudCover) * t;
    m_activeProfile.cloudFalloff    = a.cloudFalloff + (b.cloudFalloff - a.cloudFalloff) * t;
    m_activeProfile.sunDimFactor    = a.sunDimFactor + (b.sunDimFactor - a.sunDimFactor) * t;
    m_activeProfile.fogDensity      = a.fogDensity + (b.fogDensity - a.fogDensity) * t;
    m_activeProfile.horizonHazeTint = Vector3LerpRGB(a.horizonHazeTint, b.horizonHazeTint, t);
    m_activeProfile.windVector      = Vector3Lerp(a.windVector, b.windVector, t);
    m_activeProfile.turbulence      = a.turbulence + (b.turbulence - a.turbulence) * t;
    m_activeProfile.rainIntensity   = a.rainIntensity + (b.rainIntensity - a.rainIntensity) * t;
}

void WeatherSystem::Update(float deltaTime, const Vector3& cameraPos, const Vector3& aircraftVel) {

    if (m_autoWeatherEnabled && m_transitionProgress >= 1.0f) {
        m_stateTimer += deltaTime;
        if (m_stateTimer >= m_stateDuration) {
            WeatherPreset nextPreset = PickNextWeatherPreset(m_currentPreset);
            // Smooth 15-second natural atmospheric blend
            SetWeather(nextPreset, 15.0f);
            ResetStateTimer();
        }
    }

    if (m_transitionProgress < 1.0f) {
        m_transitionProgress += deltaTime / m_transitionDuration;
        if (m_transitionProgress >= 1.0f) {
            m_transitionProgress = 1.0f;
            m_currentPreset = m_targetPreset;
        }
        WeatherProfile target = GetPresetProfile(m_targetPreset);
        BlendProfiles(m_currentProfile, target, m_transitionProgress);
    }

    if (m_transitionProgress < 1.0f) {
        m_transitionProgress += deltaTime / m_transitionDuration;
        if (m_transitionProgress >= 1.0f) {
            m_transitionProgress = 1.0f;
            m_currentPreset = m_targetPreset;
        }
        WeatherProfile target = GetPresetProfile(m_targetPreset);
        BlendProfiles(m_currentProfile, target, m_transitionProgress);
    }

    // Update Rain Particles around Camera
    if (m_activeProfile.rainIntensity > 0.02f) {
        float half = m_rainBoxSize * 0.5f;
        Vector3 netVelocity = Vector3Add(
            { m_activeProfile.windVector.x, -50.0f, m_activeProfile.windVector.z },
            Vector3Scale(aircraftVel, -0.25f) // Relative visual wind
        );

        for (auto& drop : m_rainDrops) {
            drop.offset.x += netVelocity.x * deltaTime;
            drop.offset.y -= drop.speed * deltaTime;
            drop.offset.z += netVelocity.z * deltaTime;

            // Wrap within camera-relative bounding box
            if (drop.offset.y < -half) drop.offset.y += m_rainBoxSize;
            if (drop.offset.y >  half) drop.offset.y -= m_rainBoxSize;
            if (drop.offset.x < -half) drop.offset.x += m_rainBoxSize;
            if (drop.offset.x >  half) drop.offset.x -= m_rainBoxSize;
            if (drop.offset.z < -half) drop.offset.z += m_rainBoxSize;
            if (drop.offset.z >  half) drop.offset.z -= m_rainBoxSize;
        }
    }
}

void WeatherSystem::Draw3D(const Vector3& cameraPos) {
    if (m_activeProfile.rainIntensity <= 0.02f) return;

    rlDisableBackfaceCulling();
    rlDisableDepthMask();

    Color dropColor = Fade(SKYBLUE, 0.45f * m_activeProfile.rainIntensity);
    Vector3 slant = Vector3Scale(m_activeProfile.windVector, 0.04f);

    int activeCount = (int)(m_rainDrops.size() * m_activeProfile.rainIntensity);
    for (int i = 0; i < activeCount; i++) {
        const auto& drop = m_rainDrops[i];
        Vector3 start = Vector3Add(cameraPos, drop.offset);
        Vector3 end = {
            start.x + slant.x,
            start.y - drop.length,
            start.z + slant.z
        };
        DrawLine3D(start, end, dropColor);
    }

    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

std::string WeatherSystem::GetCurrentWeatherName() const {
    switch (m_currentPreset) {
        case WeatherPreset::CLEAR:         return "CLEAR";
        case WeatherPreset::PARTLY_CLOUDY: return "PARTLY CLOUDY";
        case WeatherPreset::OVERCAST:      return "OVERCAST";
        case WeatherPreset::DENSE_FOG:     return "DENSE FOG";
        case WeatherPreset::STORMY_RAIN:   return "STORMY RAIN";
        default:                           return "UNKNOWN";
    }
}