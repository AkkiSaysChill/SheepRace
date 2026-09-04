#include "engine/DayNightCycle.h"
#include <cmath>
#include <algorithm>

static Vector3 Vector3LerpRGB(Vector3 a, Vector3 b, float t) {
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

static Vector4 Vector4LerpRGBA(Vector4 a, Vector4 b, float t) {
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    };
}

DayNightCycle::DayNightCycle()
    : m_timeOfDay(12.0f), m_timeScale(0.05f), m_isPaused(false) {
    CalculateLighting();
}

void DayNightCycle::SetTime(float hours) {
    m_timeOfDay = fmodf(hours, 24.0f);
    if (m_timeOfDay < 0.0f) m_timeOfDay += 24.0f;
    CalculateLighting();
}

void DayNightCycle::Update(float deltaTime) {
    if (!m_isPaused && m_timeScale > 0.0f) {
        m_timeOfDay += (deltaTime * m_timeScale);
        if (m_timeOfDay >= 24.0f) m_timeOfDay = fmodf(m_timeOfDay, 24.0f);
        if (m_timeOfDay < 0.0f) m_timeOfDay += 24.0f;
        CalculateLighting();
    }
}

void DayNightCycle::CalculateLighting() {
    float angle = (m_timeOfDay / 24.0f) * 2.0f * PI - (PI * 0.5f);

    float sunX = cosf(angle) * 0.75f;
    float sunY = sinf(angle);
    float sunZ = cosf(angle) * 0.45f;

    Vector3 rawSunDir = Vector3Normalize({ sunX, sunY, sunZ });
    m_currentLighting.isNight = (sunY < 0.0f);

    if (!m_currentLighting.isNight) {
        m_currentLighting.lightDir = Vector3Negate(rawSunDir);
    } else {
        Vector3 rawMoonDir = Vector3Normalize({ -sunX, -sunY, -sunZ });
        m_currentLighting.lightDir = Vector3Negate(rawMoonDir);
    }

    // Palette Definitions
    const Vector4 kSunNoonColor     = { 1.0f, 0.98f, 0.92f, 1.0f };
    const Vector4 kAmbNoonColor     = { 0.35f, 0.40f, 0.48f, 1.0f };
    const Vector3 kSkyNoonColor     = { 0.50f, 0.60f, 0.85f };
    const Vector3 kEquatorNoonColor = { 0.45f, 0.45f, 0.50f };
    const Vector3 kGroundNoonColor  = { 0.20f, 0.18f, 0.18f };

    const Vector4 kSunSunsetColor     = { 1.0f, 0.52f, 0.22f, 1.0f };
    const Vector4 kAmbSunsetColor     = { 0.28f, 0.22f, 0.24f, 1.0f };
    const Vector3 kSkySunsetColor     = { 0.82f, 0.46f, 0.30f };
    const Vector3 kEquatorSunsetColor = { 0.58f, 0.32f, 0.28f };
    const Vector3 kGroundSunsetColor  = { 0.16f, 0.12f, 0.12f };

    const Vector4 kSunTwilightColor     = { 0.35f, 0.18f, 0.22f, 1.0f };
    const Vector4 kAmbTwilightColor     = { 0.14f, 0.16f, 0.24f, 1.0f };
    const Vector3 kSkyTwilightColor     = { 0.16f, 0.20f, 0.40f };
    const Vector3 kEquatorTwilightColor = { 0.18f, 0.16f, 0.28f };
    const Vector3 kGroundTwilightColor  = { 0.08f, 0.08f, 0.12f };

    const Vector4 kMoonNightColor     = { 0.18f, 0.24f, 0.38f, 1.0f };
    const Vector4 kAmbNightColor      = { 0.08f, 0.10f, 0.16f, 1.0f };
    const Vector3 kSkyNightColor      = { 0.03f, 0.05f, 0.10f };
    const Vector3 kEquatorNightColor  = { 0.05f, 0.07f, 0.12f };
    const Vector3 kGroundNightColor   = { 0.04f, 0.04f, 0.06f };

    float sunElev = sunY;

    if (sunElev > 0.25f) {
        float t = std::clamp((sunElev - 0.25f) / 0.55f, 0.0f, 1.0f);
        m_currentLighting.lightColor   = Vector4LerpRGBA(kSunSunsetColor, kSunNoonColor, t);
        m_currentLighting.ambientColor = Vector4LerpRGBA(kAmbSunsetColor, kAmbNoonColor, t);
        m_currentLighting.skyColor     = Vector3LerpRGB(kSkySunsetColor, kSkyNoonColor, t);
        m_currentLighting.equatorColor = Vector3LerpRGB(kEquatorSunsetColor, kEquatorNoonColor, t);
        m_currentLighting.groundColor  = Vector3LerpRGB(kGroundSunsetColor, kGroundNoonColor, t);
    } 
    else if (sunElev > 0.0f) {
        float t = std::clamp(sunElev / 0.25f, 0.0f, 1.0f);
        m_currentLighting.lightColor   = Vector4LerpRGBA(kSunTwilightColor, kSunSunsetColor, t);
        m_currentLighting.ambientColor = Vector4LerpRGBA(kAmbTwilightColor, kAmbSunsetColor, t);
        m_currentLighting.skyColor     = Vector3LerpRGB(kSkyTwilightColor, kSkySunsetColor, t);
        m_currentLighting.equatorColor = Vector3LerpRGB(kEquatorTwilightColor, kEquatorSunsetColor, t);
        m_currentLighting.groundColor  = Vector3LerpRGB(kGroundTwilightColor, kGroundSunsetColor, t);
    } 
    else if (sunElev > -0.15f) {
        float t = std::clamp((sunElev + 0.15f) / 0.15f, 0.0f, 1.0f);
        m_currentLighting.lightColor   = Vector4LerpRGBA(kMoonNightColor, kSunTwilightColor, t);
        m_currentLighting.ambientColor = Vector4LerpRGBA(kAmbNightColor, kAmbTwilightColor, t);
        m_currentLighting.skyColor     = Vector3LerpRGB(kSkyNightColor, kSkyTwilightColor, t);
        m_currentLighting.equatorColor = Vector3LerpRGB(kEquatorNightColor, kEquatorTwilightColor, t);
        m_currentLighting.groundColor  = Vector3LerpRGB(kGroundNightColor, kGroundTwilightColor, t);
    } 
    else {
        m_currentLighting.lightColor   = kMoonNightColor;
        m_currentLighting.ambientColor = kAmbNightColor;
        m_currentLighting.skyColor     = kSkyNightColor;
        m_currentLighting.equatorColor = kEquatorNightColor;
        m_currentLighting.groundColor  = kGroundNightColor;
    }

    m_currentLighting.cloudCover   = 0.45f;
    m_currentLighting.cloudFalloff = 1.0f;
}

void DayNightCycle::ApplyToShaders(Shader shadowShader, Shader skyShader) {
    if (shadowShader.id > 0) {
        if (int loc = GetShaderLocation(shadowShader, "lightDir"); loc != -1) {
            SetShaderValue(shadowShader, loc, &m_currentLighting.lightDir, SHADER_UNIFORM_VEC3);
        }
        if (int loc = GetShaderLocation(shadowShader, "lightColor"); loc != -1) {
            SetShaderValue(shadowShader, loc, &m_currentLighting.lightColor, SHADER_UNIFORM_VEC4);
        }
        if (int loc = GetShaderLocation(shadowShader, "ambient"); loc != -1) {
            SetShaderValue(shadowShader, loc, &m_currentLighting.ambientColor, SHADER_UNIFORM_VEC4);
        }
        if (int loc = GetShaderLocation(shadowShader, "u_skyColor"); loc != -1) {
            SetShaderValue(shadowShader, loc, &m_currentLighting.skyColor, SHADER_UNIFORM_VEC3);
        }
        if (int loc = GetShaderLocation(shadowShader, "u_equatorColor"); loc != -1) {
            SetShaderValue(shadowShader, loc, &m_currentLighting.equatorColor, SHADER_UNIFORM_VEC3);
        }
        if (int loc = GetShaderLocation(shadowShader, "u_groundColor"); loc != -1) {
            SetShaderValue(shadowShader, loc, &m_currentLighting.groundColor, SHADER_UNIFORM_VEC3);
        }
    }

    if (skyShader.id > 0) {
        Vector3 skySunVector = Vector3Negate(m_currentLighting.lightDir);
        if (int loc = GetShaderLocation(skyShader, "u_lightDir"); loc != -1) {
            SetShaderValue(skyShader, loc, &skySunVector, SHADER_UNIFORM_VEC3);
        }
        if (int loc = GetShaderLocation(skyShader, "u_skyColor"); loc != -1) {
            SetShaderValue(skyShader, loc, &m_currentLighting.skyColor, SHADER_UNIFORM_VEC3);
        }
        if (int loc = GetShaderLocation(skyShader, "u_horizonColor"); loc != -1) {
            SetShaderValue(skyShader, loc, &m_currentLighting.equatorColor, SHADER_UNIFORM_VEC3);
        }
        if (int loc = GetShaderLocation(skyShader, "u_cloudCover"); loc != -1) {
            SetShaderValue(skyShader, loc, &m_currentLighting.cloudCover, SHADER_UNIFORM_FLOAT);
        }
        if (int loc = GetShaderLocation(skyShader, "u_cloudFalloff"); loc != -1) {
            SetShaderValue(skyShader, loc, &m_currentLighting.cloudFalloff, SHADER_UNIFORM_FLOAT);
        }
    }
}