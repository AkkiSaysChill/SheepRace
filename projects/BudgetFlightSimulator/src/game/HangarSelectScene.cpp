#include "game/HangarSelectScene.h"
#include "engine/Engine.h"
#include "engine/SceneManager.h"
#include "game/AircraftDatabase.h"
#include "game/FlightSession.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>

HangarSelectScene::HangarSelectScene() : m_engine(nullptr) {}
HangarSelectScene::~HangarSelectScene() {}

void HangarSelectScene::Initialize(Engine *engine) {
  Scene3D::Initialize(engine);
  m_engine = engine;

  auto &assetLoader = m_engine->GetAssetLoader();
  auto &renderer = m_engine->GetRenderer();
  assetLoader.loadPackedFile("shaders.bin");
  assetLoader.loadPackedFile("planes.woosh");

  // Camera setup
  m_camera.position = {0.0f, 4.0f, 18.0f};
  m_camera.target = {0.0f, 1.2f, 0.0f};
  m_camera.up = {0.0f, 1.0f, 0.0f};
  m_camera.fovy = 45.0f;
  m_camera.projection = CAMERA_PERSPECTIVE;

  // =========================================================================
  // HANGAR LIGHTING SETUP
  // =========================================================================
  Shader shadowShader = renderer.GetShadowShader();
  m_engine->PostProcessingActive = false;

  // 1. Directional Sun/Spotlight
  Vector3 lightDir = Vector3Normalize({0.35f, -0.85f, 0.40f});
  if (int loc = GetShaderLocation(shadowShader, "lightDir"); loc != -1) {
    SetShaderValue(shadowShader, loc, &lightDir, SHADER_UNIFORM_VEC3);
  }

  // 2. Light Color (Warm Studio Lighting)
  Vector4 lightColor = {1.0f, 0.98f, 0.92f, 1.0f};
  if (int loc = GetShaderLocation(shadowShader, "lightColor"); loc != -1) {
    SetShaderValue(shadowShader, loc, &lightColor, SHADER_UNIFORM_VEC4);
  }

  // 3. Ambient Baseline
  if (int loc = GetShaderLocation(shadowShader, "ambient"); loc != -1) {
    float ambientColor[4] = {0.45f, 0.45f, 0.55f, 1.0f};
    SetShaderValue(shadowShader, loc, ambientColor, SHADER_UNIFORM_VEC4);
  }

  // 4. Tri-Color GI Environment
  Vector3 skyColor = {0.50f, 0.60f, 0.85f};
  Vector3 equatorColor = {0.45f, 0.45f, 0.50f};
  Vector3 groundColor = {0.20f, 0.18f, 0.18f};

  if (int loc = GetShaderLocation(shadowShader, "u_skyColor"); loc != -1)
    SetShaderValue(shadowShader, loc, &skyColor, SHADER_UNIFORM_VEC3);
  if (int loc = GetShaderLocation(shadowShader, "u_equatorColor"); loc != -1)
    SetShaderValue(shadowShader, loc, &equatorColor, SHADER_UNIFORM_VEC3);
  if (int loc = GetShaderLocation(shadowShader, "u_groundColor"); loc != -1)
    SetShaderValue(shadowShader, loc, &groundColor, SHADER_UNIFORM_VEC3);

  // Map diffuse color uniform for imported Blender materials
  shadowShader.locs[SHADER_LOC_COLOR_DIFFUSE] =
      GetShaderLocation(shadowShader, "colDiffuse");
  shadowShader.locs[SHADER_LOC_MAP_ALBEDO] =
      GetShaderLocation(shadowShader, "texture0");

  // Load aircraft database
  auto xmlData = assetLoader.getAssetData("aircraft.xml");
  if (!xmlData.empty()) {
    AircraftDatabase::GetInstance().LoadFromMemory(xmlData.data(),
                                                   xmlData.size());
  } else {
    AircraftDatabase::GetInstance().LoadFromFile("assets/data/aircraft.xml");
  }

  m_planeIds = AircraftDatabase::GetInstance().GetAllPlaneIds();

  m_selectedIndex = 0;
  for (size_t i = 0; i < m_planeIds.size(); ++i) {
    if (m_planeIds[i] == FlightSession::GetInstance().selectedPlaneId) {
      m_selectedIndex = i;
      break;
    }
  }

  LoadCurrentAircraft();
}

void HangarSelectScene::LoadCurrentAircraft() {
  if (m_planeIds.empty())
    return;

  std::string planeId = m_planeIds[m_selectedIndex];
  if (!AircraftDatabase::GetInstance().GetStats(planeId, m_currentStats))
    return;

  auto &assetLoader = m_engine->GetAssetLoader();
  auto &renderer = m_engine->GetRenderer();
  Shader shadowShader = renderer.GetShadowShader();

  if (m_modelLoaded) {
    UnloadModel(m_currentModel);
    m_modelLoaded = false;
  }

  auto planeData = assetLoader.getAssetData(m_currentStats.modelFile);
  if (!planeData.empty()) {
    m_currentModel = assetLoader.LoadModelFromAssetData(planeData, ".glb");
    m_modelLoaded = true;
    for (int i = 0; i < m_currentModel.materialCount; ++i) {
      m_currentModel.materials[i].shader = shadowShader;

      m_currentModel.materials[i].maps[MATERIAL_MAP_ALBEDO].color.a = 255;
    }
  } else {
    m_currentModel = LoadModelFromMesh(GenMeshCube(2.0f, 0.6f, 4.5f));
    m_currentModel.materials[0].shader = shadowShader;
    m_currentModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = LIGHTGRAY;
    m_modelLoaded = true;
  }

  // Auto-fit turntable camera
  BoundingBox box = GetModelBoundingBox(m_currentModel);
  float width = fabsf(box.max.x - box.min.x);
  float height = fabsf(box.max.y - box.min.y);
  float length = fabsf(box.max.z - box.min.z);
  float maxDim = fmaxf(width, fmaxf(height, length));
  if (maxDim < 1.0f)
    maxDim = 5.0f;

  m_camDistance = maxDim * 2.2f;
  m_camHeight = fmaxf(height * 0.8f, 1.5f);
  m_camera.target = {0.0f, height * 0.4f, 0.0f};
}

void HangarSelectScene::Update(float deltaTime) {
  if (m_planeIds.empty())
    return;

  m_rotationAngle += 25.0f * deltaTime;
  if (m_rotationAngle >= 360.0f)
    m_rotationAngle -= 360.0f;

  float rad = m_rotationAngle * DEG2RAD;
  m_camera.position = {sinf(rad) * m_camDistance, m_camHeight,
                       cosf(rad) * m_camDistance};

  Camera &engineCam = m_engine->GetCamera();
  engineCam = m_camera;

  if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
    m_selectedIndex = (m_selectedIndex + 1) % m_planeIds.size();
    LoadCurrentAircraft();
  }
  if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
    m_selectedIndex =
        (m_selectedIndex + m_planeIds.size() - 1) % m_planeIds.size();
    LoadCurrentAircraft();
  }

  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
    FlightSession::GetInstance().selectedPlaneId = m_planeIds[m_selectedIndex];
    m_engine->GetSceneManager().LoadScene("FlightScene");
  }

  if (IsKeyPressed(KEY_ESCAPE)) {
    m_engine->GetSceneManager().LoadScene("MainMenuScene");
  }
}

void HangarSelectScene::Draw3D() {

  if (m_modelLoaded) {
    DrawModel(m_currentModel, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
  }

  // Draw transparent rings AFTER the solid model
  DrawCircle3D({0.0f, 0.01f, 0.0f}, m_camDistance * 0.65f, {1.0f, 0.0f, 0.0f},
               90.0f, Fade(LIME, 0.25f));
}

void HangarSelectScene::DrawStatBar(const char *label, float value,
                                    float maxValue, int x, int y, int width) {
  DrawText(label, x, y, 14, LIGHTGRAY);
  int barY = y + 16;
  DrawRectangle(x, barY, width, 8, Fade(DARKGRAY, 0.6f));
  float pct = Clamp(value / maxValue, 0.0f, 1.0f);
  DrawRectangle(x, barY, (int)(width * pct), 8, LIME);
}

void HangarSelectScene::Draw() {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  DrawText("HANGAR // AIRCRAFT SELECTION", 30, 25, 24, LIME);
  DrawText(TextFormat("AIRCRAFT %d / %d", (int)m_selectedIndex + 1,
                      (int)m_planeIds.size()),
           30, 55, 14, GRAY);

  int cardW = 340;
  int cardH = 340;
  int cardX = 30;
  int cardY = 90;

  DrawRectangle(cardX, cardY, cardW, cardH, Fade(BLACK, 0.75f));
  DrawRectangleLines(cardX, cardY, cardW, cardH, LIME);

  DrawText(m_currentStats.name.c_str(), cardX + 16, cardY + 16, 20, YELLOW);
  DrawText(TextFormat("ID: %s", m_currentStats.id.c_str()), cardX + 16,
           cardY + 40, 13, GRAY);

  int statY = cardY + 70;
  int barW = cardW - 32;

  DrawStatBar(
      TextFormat("TOP SPEED: %.0f KM/H", m_currentStats.maxThrustSpeed * 3.6f),
      m_currentStats.maxThrustSpeed, 200.0f, cardX + 16, statY, barW);
  DrawStatBar(TextFormat("ACCELERATION / THRUST: %.1f",
                         m_currentStats.throttleResponse),
              m_currentStats.throttleResponse, 20.0f, cardX + 16, statY + 36,
              barW);
  DrawStatBar(
      TextFormat("AGILITY (ROLL / PITCH): %.2f",
                 (m_currentStats.rollRate + m_currentStats.pitchRate) * 0.5f),
      m_currentStats.rollRate, 3.5f, cardX + 16, statY + 72, barW);
  DrawStatBar(TextFormat("STALL RESISTANCE: %.0f KM/H",
                         m_currentStats.stallSpeedThreshold * 3.6f),
              35.0f - m_currentStats.stallSpeedThreshold, 35.0f, cardX + 16,
              statY + 108, barW);
  DrawStatBar(
      TextFormat("GLIDE EFFICIENCY: %.1f", 3.0f - m_currentStats.glideSinkRate),
      3.0f - m_currentStats.glideSinkRate, 3.0f, cardX + 16, statY + 144, barW);

  DrawRectangle(screenW / 2 - 220, screenH - 65, 440, 45, Fade(BLACK, 0.8f));
  DrawRectangleLines(screenW / 2 - 220, screenH - 65, 440, 45, DARKGRAY);
  DrawText("[LEFT / RIGHT / A / D] Prev / Next Plane", screenW / 2 - 200,
           screenH - 52, 14, RAYWHITE);
  DrawText("[ENTER] Deploy Flight  |  [ESC] Menu", screenW / 2 - 200,
           screenH - 34, 14, YELLOW);
}

void HangarSelectScene::Shutdown() {
  if (m_modelLoaded) {
    UnloadModel(m_currentModel);
    m_modelLoaded = false;
  }
}
