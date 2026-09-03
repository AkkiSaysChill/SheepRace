#include "game/BudgetFlightSimulatorScene.h"
#include "engine/DrawableComponent.h"
#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "game/AircraftDatabase.h"
#include "game/FlightSession.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

BudgetFlightSimulatorScene::BudgetFlightSimulatorScene() : m_engine(nullptr) {}

BudgetFlightSimulatorScene::~BudgetFlightSimulatorScene() {
    Shutdown();
}

void BudgetFlightSimulatorScene::SwitchAircraft(const std::string &planeId) {
    AircraftStats newStats;
    if (!AircraftDatabase::GetInstance().GetStats(planeId, newStats))
        return;

    m_aircraft.SetStats(newStats);

    auto &assetLoader = m_engine->GetAssetLoader();
    auto &renderer = m_engine->GetRenderer();
    Shader shadowShader = renderer.GetShadowShader();

    shadowShader.locs[SHADER_LOC_COLOR_DIFFUSE] =
        GetShaderLocation(shadowShader, "colDiffuse");
    shadowShader.locs[SHADER_LOC_MAP_ALBEDO] =
        GetShaderLocation(shadowShader, "texture0");

    if (m_modelLoaded) {
        UnloadModel(m_planeModel);
        m_modelLoaded = false;
    }

    auto planeData = assetLoader.getAssetData(newStats.modelFile);
    if (!planeData.empty()) {
        m_planeModel = assetLoader.LoadModelFromAssetData(planeData, ".glb");
        m_modelLoaded = true;

        for (int i = 0; i < m_planeModel.materialCount; i++) {
            m_planeModel.materials[i].shader = shadowShader;
        }
    } else {
        m_planeModel = LoadModelFromMesh(GenMeshCube(1.5f, 0.5f, 4.0f));
        m_planeModel.materials[0].shader = shadowShader;
        m_planeModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = GRAY;
        m_modelLoaded = true;
    }

    BoundingBox modelBounds = GetModelBoundingBox(m_planeModel);
    m_flightCamera.AutoAdjustForModel(modelBounds);

    if (m_planeObject) {
        auto drawable = m_planeObject->GetComponent<DrawableComponent>();
        if (drawable) {
            drawable->GetModelRef() = m_planeModel;
        } else {
            m_planeObject->AddComponent<DrawableComponent>(m_planeModel);
        }
    }
}

void BudgetFlightSimulatorScene::Initialize(Engine *engine) {
    Scene3D::Initialize(engine);
    m_engine = engine;

    m_terrain.LoadWorld(m_engine, "island_world.glb");
    m_ocean.Initialize(m_engine, 260000.0f);

    // Start of the West Island runway (Y = 13.5m)
    m_aircraft.Reset({ -2350.0f, 13.5f, 150.0f });
    m_throttle = 0.0f;

    m_flightCamera.Initialize(55.0f, 15.0f);

    auto &gameObjectManager = m_engine->GetGameObjectManager();
    auto &assetLoader = m_engine->GetAssetLoader();
    auto &renderer = m_engine->GetRenderer();

    assetLoader.loadPackedFile("shaders.bin");
    assetLoader.loadPackedFile("planes.woosh");

    Shader shadowShader = renderer.GetShadowShader();
    m_engine->PostProcessingActive = false;

    Vector3 lightDir = Vector3Normalize({0.35f, -0.85f, 0.40f});
    if (int loc = GetShaderLocation(shadowShader, "lightDir"); loc != -1) {
        SetShaderValue(shadowShader, loc, &lightDir, SHADER_UNIFORM_VEC3);
    }

    Vector4 lightColor = {1.0f, 0.98f, 0.92f, 1.0f};
    if (int loc = GetShaderLocation(shadowShader, "lightColor"); loc != -1) {
        SetShaderValue(shadowShader, loc, &lightColor, SHADER_UNIFORM_VEC4);
    }

    if (int loc = GetShaderLocation(shadowShader, "ambient"); loc != -1) {
        float ambientColor[4] = {0.35f, 0.40f, 0.48f, 1.0f};
        SetShaderValue(shadowShader, loc, ambientColor, SHADER_UNIFORM_VEC4);
    }

    Vector3 skyColor = {0.50f, 0.60f, 0.85f};
    Vector3 equatorColor = {0.45f, 0.45f, 0.50f};
    Vector3 groundColor = {0.20f, 0.18f, 0.18f};

    if (int loc = GetShaderLocation(shadowShader, "u_skyColor"); loc != -1)
        SetShaderValue(shadowShader, loc, &skyColor, SHADER_UNIFORM_VEC3);
    if (int loc = GetShaderLocation(shadowShader, "u_equatorColor"); loc != -1)
        SetShaderValue(shadowShader, loc, &equatorColor, SHADER_UNIFORM_VEC3);
    if (int loc = GetShaderLocation(shadowShader, "u_groundColor"); loc != -1)
        SetShaderValue(shadowShader, loc, &groundColor, SHADER_UNIFORM_VEC3);

    // Raylib Fog Parameters (Tuned for ~6000m tree fade)
    float fogDensity = 0.00032f;
    Vector4 fogColor = { 0.50f, 0.60f, 0.85f, 1.0f }; // Seamlessly matches skyColor

    if (int loc = GetShaderLocation(shadowShader, "fogDensity"); loc != -1) {
        SetShaderValue(shadowShader, loc, &fogDensity, SHADER_UNIFORM_FLOAT);
    }
    if (int loc = GetShaderLocation(shadowShader, "fogColor"); loc != -1) {
        SetShaderValue(shadowShader, loc, &fogColor, SHADER_UNIFORM_VEC4);
    }

    m_planeObject = std::make_shared<GameObject>();
    m_planeObject->name = "PlayerAircraft";
    m_planeObject->position = m_aircraft.position;
    gameObjectManager.AddGameObject(m_planeObject);

    auto xmlData = assetLoader.getAssetData("aircraft.xml");
    if (!xmlData.empty()) {
        AircraftDatabase::GetInstance().LoadFromMemory(xmlData.data(), xmlData.size());
    } else {
        AircraftDatabase::GetInstance().LoadFromFile("assets/data/aircraft.xml");
    }

    std::string startingPlane = FlightSession::GetInstance().selectedPlaneId;
    const auto &ids = AircraftDatabase::GetInstance().GetAllPlaneIds();
    if (!ids.empty()) {
        m_currentPlaneIndex = 0;
        SwitchAircraft(startingPlane.empty() ? ids[0] : startingPlane);
    }
}

void BudgetFlightSimulatorScene::Update(float deltaTime) {
    if (IsKeyPressed(KEY_F)) {
        m_isFrozen = !m_isFrozen;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        m_engine->GetSceneManager().LoadScene("HangarScene");
    }

    const auto &planeIds = AircraftDatabase::GetInstance().GetAllPlaneIds();

    if (IsKeyPressed(KEY_ONE))   SwitchAircraft("trainer");
    if (IsKeyPressed(KEY_TWO))   SwitchAircraft("jet_fighter");
    if (IsKeyPressed(KEY_THREE)) SwitchAircraft("heavy_cargo");

    if (!planeIds.empty()) {
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
            m_currentPlaneIndex = (m_currentPlaneIndex + 1) % planeIds.size();
            SwitchAircraft(planeIds[m_currentPlaneIndex]);
        }
        if (IsKeyPressed(KEY_LEFT_BRACKET)) {
            m_currentPlaneIndex = (m_currentPlaneIndex + planeIds.size() - 1) % planeIds.size();
            SwitchAircraft(planeIds[m_currentPlaneIndex]);
        }
    }

    if (IsKeyPressed(KEY_F5)) {
        auto xmlData = m_engine->GetAssetLoader().getAssetData("aircraft.xml");
        if (!xmlData.empty()) {
            AircraftDatabase::GetInstance().LoadFromMemory(xmlData.data(), xmlData.size());
        } else {
            AircraftDatabase::GetInstance().LoadFromFile("assets/data/aircraft.xml");
        }
        if (!planeIds.empty()) {
            SwitchAircraft(planeIds[m_currentPlaneIndex]);
        }
    }

    if (!m_isFrozen) {
        FlightInput input;
        if (IsKeyDown(KEY_W)) input.pitch = -1.0f;
        if (IsKeyDown(KEY_S)) input.pitch = 1.0f;
        if (IsKeyDown(KEY_A)) input.roll = -1.0f;
        if (IsKeyDown(KEY_D)) input.roll = 1.0f;
        if (IsKeyDown(KEY_Q)) input.yaw = 1.0f;
        if (IsKeyDown(KEY_E)) input.yaw = -1.0f;
        input.brake = IsKeyDown(KEY_SPACE);

        if (IsKeyDown(KEY_LEFT_SHIFT))   m_throttle += 0.35f * deltaTime;
        if (IsKeyDown(KEY_LEFT_CONTROL)) m_throttle -= 0.35f * deltaTime;
        m_throttle = Clamp(m_throttle, 0.0f, 1.0f);
        input.throttle = m_throttle;

        m_aircraft.Update(input, deltaTime);
    }

    m_flightCamera.Update(m_aircraft, deltaTime);

    Vector3 skySun = m_engine->GetRenderer().GetSkySunVector();
    m_ocean.Update(deltaTime, m_flightCamera.GetRaylibCamera(), skySun);

    // Keep camera view position uniform updated for distance fog
    Camera3D flyCam = m_flightCamera.GetRaylibCamera();
    Shader shadowShader = m_engine->GetRenderer().GetShadowShader();
    if (int loc = GetShaderLocation(shadowShader, "viewPos"); loc != -1) {
        SetShaderValue(shadowShader, loc, &flyCam.position, SHADER_UNIFORM_VEC3);
    }

    // Synchronize the plane's GameObject for the shadow map pass
    if (m_planeObject) {
        m_planeObject->position = m_aircraft.position;
        m_planeObject->rotation = m_aircraft.orientation;

        auto drawable = m_planeObject->GetComponent<DrawableComponent>();
        if (drawable && m_modelLoaded) {
            drawable->GetModelRef() = m_planeModel;
        }
    }

    float terrainGroundHeight =
        m_terrain.GetHeightAt(m_aircraft.position.x, m_aircraft.position.z) + 1.5f;

    if (m_aircraft.position.y <= terrainGroundHeight) {
        m_aircraft.position.y = terrainGroundHeight;
        m_aircraft.isOnGround = true;
        if (m_aircraft.velocity.y < 0.0f)
            m_aircraft.velocity.y = 0.0f;
    }

    Camera &engineCam = m_engine->GetCamera();
    engineCam.position   = flyCam.position;
    engineCam.target     = flyCam.target;
    engineCam.up         = flyCam.up;
    engineCam.fovy       = flyCam.fovy;
    engineCam.projection = flyCam.projection;
}

void BudgetFlightSimulatorScene::Draw3D() {
    rlEnableDepthTest();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();

    // Setup Frustum
    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();
    double aspect = (double)GetScreenWidth() / (double)GetScreenHeight();
    double top = 2.0 * tan(m_flightCamera.GetRaylibCamera().fovy * 0.5 * DEG2RAD);
    double right = top * aspect;
    rlFrustum(-right, right, -top, top, 2.0, 15000.0);
    rlMatrixMode(RL_MODELVIEW);

    // 1. Render terrain and props with shadowShader
    Shader shadowShader = m_engine->GetRenderer().GetShadowShader();
    m_terrain.Draw(shadowShader);

    Camera3D flyCam = m_flightCamera.GetRaylibCamera();
    m_terrain.DrawProps(flyCam);
    m_ocean.Draw();

    // 2. Render Aircraft
    if (m_modelLoaded) {
        Matrix planeTransform = MatrixMultiply(
            QuaternionToMatrix(m_aircraft.orientation),
            MatrixTranslate(m_aircraft.position.x, m_aircraft.position.y, m_aircraft.position.z)
        );
        
        for (int i = 0; i < m_planeModel.meshCount; ++i) {
            DrawMesh(m_planeModel.meshes[i], m_planeModel.materials[m_planeModel.meshMaterial[i]], planeTransform);
        }
    }

    // Flush and reset 3D render pipeline cleanly
    rlDrawRenderBatchActive();

    rlMatrixMode(RL_PROJECTION);
    rlPopMatrix();
    rlMatrixMode(RL_MODELVIEW);
}

void BudgetFlightSimulatorScene::Draw() {
    DrawRectangle(20, 20, 280, 160, Fade(BLACK, 0.6f));
    DrawRectangleLines(20, 20, 280, 160, LIME);

    DrawText(TextFormat("PLANE:    %s", m_aircraft.stats.name.c_str()), 35, 30, 16, YELLOW);
    DrawText(TextFormat("AIRSPEED: %.0f KM/H", m_aircraft.GetAirspeedKmh()), 35, 52, 18, LIME);
    DrawText(TextFormat("ALTITUDE: %.1f M", m_aircraft.GetAltitudeMeters()), 35, 74, 18, LIME);
    DrawText(TextFormat("HEADING:  %.0f DEG", m_aircraft.GetHeading()), 35, 96, 18, LIME);
    DrawText(TextFormat("THROTTLE: %.0f%%", m_throttle * 100.0f), 35, 118, 18, LIME);
    DrawText(TextFormat("G-FORCE:  %.1f G", m_aircraft.currentGForce), 35, 140, 18,
             m_aircraft.currentGForce > 3.0f ? RED : LIME);

    DrawText("Cycle: [ [ ] and [ ] ] | Hot-Reload: [ F5 ]", 20,
             GetScreenHeight() - 30, 16, RAYWHITE);

    if (m_aircraft.isStalling) {
        DrawText("!! STALL WARNING !!", GetScreenWidth() / 2 - 130, 80, 26, RED);
    }
}

void BudgetFlightSimulatorScene::Shutdown() {
    // Reset fog density to 0 on exit so subsequent scenes/games don't render fog
    Shader shadowShader = m_engine->GetRenderer().GetShadowShader();
    float zeroFog = 0.0f;
    if (int loc = GetShaderLocation(shadowShader, "fogDensity"); loc != -1) {
        SetShaderValue(shadowShader, loc, &zeroFog, SHADER_UNIFORM_FLOAT);
    }

    if (m_modelLoaded) {
        UnloadModel(m_planeModel);
        m_modelLoaded = false;
    }
    m_ocean.Unload();
}