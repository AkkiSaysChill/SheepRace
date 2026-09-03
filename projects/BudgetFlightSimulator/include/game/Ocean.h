#pragma once
#include "raylib.h"
#include "raymath.h"

class Ocean {
public:
    Ocean();
    ~Ocean();

    void Initialize(class Engine* engine, float size = 26000.0f);
    void Update(float deltaTime, Camera3D camera, Vector3 sunDirection);
    void Draw();
    void Unload();

    // Weather parameter adjustments
    void SetWaveParameters(float steepness, float length, float speed);

private:
    class Engine* m_engine = nullptr;
    Model m_oceanModel = { 0 };
    Shader m_oceanShader = { 0 };
    bool m_isLoaded = false;
    float m_time = 0.0f;

    // Weather settings
    float m_waveSteepness = 0.22f;
    float m_waveLength = 80.0f;
    float m_waveSpeed = 1.2f;

    int m_timeLoc = -1;
    int m_viewPosLoc = -1;
    int m_lightDirLoc = -1;
    int m_resLoc = -1;
    int m_nearLoc = -1;
    int m_farLoc = -1;
    int m_waveSteepnessLoc = -1;
    int m_waveLengthLoc = -1;
    int m_waveSpeedLoc = -1;
};