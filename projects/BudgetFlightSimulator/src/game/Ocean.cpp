#include "game/Ocean.h"
#include "engine/Engine.h"
#include "rlgl.h"
#include "raymath.h"
#include <iostream>
#include <cstdio>
#include <vector>

Ocean::Ocean() {}

Ocean::~Ocean() {
    Unload();
}

void Ocean::SetWaveParameters(float steepness, float length, float speed) {
    m_waveSteepness = steepness;
    m_waveLength = length;
    m_waveSpeed = speed;
}

void Ocean::Initialize(Engine* engine, float size) {
    m_engine = engine;
    auto& assetLoader = engine->GetAssetLoader();
    assetLoader.loadPackedFile("shaders.bin");

    std::vector<char> vsData = assetLoader.getAssetData("glsl330/ocean.vs");
    std::vector<char> fsData = assetLoader.getAssetData("glsl330/ocean.fs");

    if (!vsData.empty() && !fsData.empty()) {
        SaveFileData("temp_ocean.vs", vsData.data(), (unsigned int)vsData.size());
        SaveFileData("temp_ocean.fs", fsData.data(), (unsigned int)fsData.size());
        m_oceanShader = LoadShader("temp_ocean.vs", "temp_ocean.fs");
        remove("temp_ocean.vs");
        remove("temp_ocean.fs");

        m_oceanShader.locs[SHADER_LOC_MATRIX_MVP]   = GetShaderLocation(m_oceanShader, "mvp");
        m_oceanShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(m_oceanShader, "matModel");
        m_oceanShader.locs[SHADER_LOC_VECTOR_VIEW]  = GetShaderLocation(m_oceanShader, "viewPos");

        m_timeLoc          = GetShaderLocation(m_oceanShader, "time");
        m_viewPosLoc       = GetShaderLocation(m_oceanShader, "viewPos");
        m_lightDirLoc      = GetShaderLocation(m_oceanShader, "u_lightDir");
        m_resLoc           = GetShaderLocation(m_oceanShader, "resolution");
        m_nearLoc          = GetShaderLocation(m_oceanShader, "nearPlane");
        m_farLoc           = GetShaderLocation(m_oceanShader, "farPlane");
        m_waveSteepnessLoc = GetShaderLocation(m_oceanShader, "u_waveSteepness");
        m_waveLengthLoc    = GetShaderLocation(m_oceanShader, "u_waveLength");
        m_waveSpeedLoc     = GetShaderLocation(m_oceanShader, "u_waveSpeed");

        // Set up texture slots
        m_oceanShader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(m_oceanShader, "depthMap");
        m_oceanShader.locs[SHADER_LOC_MAP_ALBEDO]    = GetShaderLocation(m_oceanShader, "screenMap");

        // High tessellation mesh plane for stable elevation and vertex wave handling
        Mesh oceanMesh = GenMeshPlane(size, size, 90, 91);
        m_oceanModel = LoadModelFromMesh(oceanMesh);
        m_oceanModel.materials[0].shader = m_oceanShader;

        m_isLoaded = true;
        std::cout << "[Ocean] Initialized Realistic Gerstner Depth Ocean.\n";
    }
}

void Ocean::Update(float deltaTime, Camera3D camera, Vector3 sunDirection) {
    if (!m_isLoaded) return;

    m_time += deltaTime;

    Vector3 normSun = Vector3Normalize(sunDirection);
    float res[2] = { (float)GetScreenWidth(), (float)GetScreenHeight() };

    SetShaderValue(m_oceanShader, m_timeLoc, &m_time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_oceanShader, m_viewPosLoc, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_oceanShader, m_lightDirLoc, &normSun, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_oceanShader, m_resLoc, res, SHADER_UNIFORM_VEC2);

    if (m_waveSteepnessLoc != -1) SetShaderValue(m_oceanShader, m_waveSteepnessLoc, &m_waveSteepness, SHADER_UNIFORM_FLOAT);
    if (m_waveLengthLoc != -1)    SetShaderValue(m_oceanShader, m_waveLengthLoc, &m_waveLength, SHADER_UNIFORM_FLOAT);
    if (m_waveSpeedLoc != -1)     SetShaderValue(m_oceanShader, m_waveSpeedLoc, &m_waveSpeed, SHADER_UNIFORM_FLOAT);
}

void Ocean::Draw() {
    if (!m_isLoaded) return;

    Vector3 oceanPosition = { 0.0f, 1.0f, 2750.0f };
    Matrix modelTransform = MatrixTranslate(oceanPosition.x, oceanPosition.y, oceanPosition.z);

    // Keep shader matModel synced with the draw translation
    int modelLoc = m_oceanShader.locs[SHADER_LOC_MATRIX_MODEL];
    if (modelLoc != -1) {
        SetShaderValueMatrix(m_oceanShader, modelLoc, modelTransform);
    }

    rlDisableBackfaceCulling();
    DrawModel(m_oceanModel, oceanPosition, 1.0f, WHITE);
    rlEnableBackfaceCulling();
}

void Ocean::Unload() {
    if (m_isLoaded) {
        UnloadModel(m_oceanModel);
        UnloadShader(m_oceanShader);
        m_isLoaded = false;
    }
}