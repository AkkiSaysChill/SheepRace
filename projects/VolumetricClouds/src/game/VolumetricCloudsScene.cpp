#include "game/VolumetricCloudsScene.h"
#include "engine/DrawableComponent.h"
#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/PhysicsComponent.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

VolumetricCloudsScene::VolumetricCloudsScene()
    : m_engine(nullptr), m_modelLoaded(false) {}

VolumetricCloudsScene::~VolumetricCloudsScene() { Shutdown(); }

void VolumetricCloudsScene::Initialize(Engine *engine) {
  Scene3D::Initialize(engine);
  m_engine = engine;

  auto &gameObjectManager = m_engine->GetGameObjectManager();
  auto &renderer = m_engine->GetRenderer();
  Shader shadowShader = renderer.GetShadowShader();

  // Enable volumetric clouds for this scene
  renderer.VolumetricCloudsActive = true;

  // Lock cursor & set initial camera for First-Person Free-Fly
  DisableCursor();
  Camera &cam = m_engine->GetCamera();
  cam.position = {0.0f, 5.0f, 15.0f};
  cam.target = {0.0f, 8.0f, 0.0f};
  cam.up = {0.0f, 1.0f, 0.0f};
  cam.fovy = 60.0f;

  // 1. Setup Day/Night Cycle (Default: 10:30 AM, relaxed 0.05 speed)
  auto &dayNight = m_engine->GetDayNightCycle();
  dayNight.SetTime(10.5f);
  dayNight.SetTimeScale(0.05f);

  // 2. Setup Weather System
  auto &weatherSys = m_engine->GetWeatherSystem();
  weatherSys.SetWeather(WeatherPreset::PARTLY_CLOUDY, 1.0f);
  weatherSys.SetAutoWeatherEnabled(true);

  // 3. Create a Sample Lit Object with Shadow Shader
  m_sampleObject = std::make_shared<GameObject>();
  m_sampleObject->name = "Sample Cube";
  m_sampleObject->position = {0.0f, 1.0f, 0.0f};

  m_sampleModel = LoadModelFromMesh(GenMeshCube(2.0f, 2.0f, 2.0f));
  for (int i = 0; i < m_sampleModel.materialCount; ++i) {
    m_sampleModel.materials[i].shader = shadowShader;
  }
  m_modelLoaded = true;

  m_sampleObject->AddComponent<DrawableComponent>(m_sampleModel);

  // Optional Physics Box
  btCollisionShape *shape = new btBoxShape(btVector3(1.0f, 1.0f, 1.0f));
  m_sampleObject->AddComponent<PhysicsComponent>(
      &m_engine->GetPhysicsSystem(), shape, PhysicsShapeType::BOX, 1.0f,
      m_sampleObject->position, QuaternionIdentity());

  gameObjectManager.AddGameObject(m_sampleObject);
}

void VolumetricCloudsScene::Update(float deltaTime) {
  // 1. Weather Controls (K: Prev, L: Next)
  auto &weatherSys = m_engine->GetWeatherSystem();
  if (IsKeyPressed(KEY_L)) {
    weatherSys.CycleNextWeather(4.0f);
    weatherSys.ResetStateTimer();
  }
  if (IsKeyPressed(KEY_K)) {
    int prev =
        ((int)weatherSys.GetCurrentPreset() + (int)WeatherPreset::COUNT - 1) %
        (int)WeatherPreset::COUNT;
    weatherSys.SetWeather((WeatherPreset)prev, 4.0f);
    weatherSys.ResetStateTimer();
  }

  Camera &cam = m_engine->GetCamera();

  // --- First-Person Free-Fly Controller ---
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    DisableCursor();
  }
  if (IsKeyPressed(KEY_TAB)) {
    if (IsCursorHidden())
      EnableCursor();
    else
      DisableCursor();
  }

  if (IsCursorHidden()) {
    float moveSpeed = 35.0f * deltaTime;
    if (IsKeyDown(KEY_LEFT_SHIFT))
      moveSpeed *= 5.0f; // Fast boost to fly up into clouds

    Vector3 forward =
        Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 right = Vector3Normalize(
        Vector3CrossProduct(forward, Vector3{0.0f, 1.0f, 0.0f}));

    // Mouse Look (Yaw & Pitch)
    Vector2 mouseDelta = GetMouseDelta();
    const float sensitivity = 0.0025f;

    forward =
        Vector3Transform(forward, MatrixRotateY(-mouseDelta.x * sensitivity));
    right = Vector3Normalize(
        Vector3CrossProduct(forward, Vector3{0.0f, 1.0f, 0.0f}));

    Vector3 pitchedForward = Vector3Transform(
        forward, MatrixRotate(right, -mouseDelta.y * sensitivity));
    // Clamp pitch so camera doesn't flip upside down
    if (fabsf(Vector3DotProduct(pitchedForward, Vector3{0.0f, 1.0f, 0.0f})) <
        0.99f) {
      forward = pitchedForward;
    }

    // Movement (WASD + Q/E or Space/Ctrl)
    if (IsKeyDown(KEY_W))
      cam.position = Vector3Add(cam.position, Vector3Scale(forward, moveSpeed));
    if (IsKeyDown(KEY_S))
      cam.position =
          Vector3Subtract(cam.position, Vector3Scale(forward, moveSpeed));
    if (IsKeyDown(KEY_D))
      cam.position = Vector3Add(cam.position, Vector3Scale(right, moveSpeed));
    if (IsKeyDown(KEY_A))
      cam.position =
          Vector3Subtract(cam.position, Vector3Scale(right, moveSpeed));
    if (IsKeyDown(KEY_E) || IsKeyDown(KEY_SPACE))
      cam.position.y += moveSpeed;
    if (IsKeyDown(KEY_Q) || IsKeyDown(KEY_LEFT_CONTROL))
      cam.position.y -= moveSpeed;

    cam.target = Vector3Add(cam.position, forward);
    cam.up = {0.0f, 1.0f, 0.0f};
  }

  weatherSys.Update(deltaTime, cam.position, Vector3Zero());

  // 2. Day/Night Controls (O: -1hr, P: +1hr)
  auto &dayNight = m_engine->GetDayNightCycle();
  dayNight.Update(deltaTime);

  if (IsKeyDown(KEY_P))
    dayNight.SetTime(dayNight.GetTime() + 1.5f * deltaTime);
  if (IsKeyDown(KEY_O))
    dayNight.SetTime(dayNight.GetTime() - 1.5f * deltaTime);

  // 3. Shader Atmospheric Forwarding
  Shader shadowShader = m_engine->GetRenderer().GetShadowShader();
  Shader skyShader = m_engine->GetRenderer().GetSkyShader();

  dayNight.ApplyToShaders(shadowShader, skyShader);

  const auto &weatherProf = weatherSys.GetActiveProfile();
  Vector4 dimmedSun = Vector4Scale(dayNight.GetCurrentLighting().lightColor,
                                   weatherProf.sunDimFactor);
  int lightColLoc = GetShaderLocation(shadowShader, "lightColor");
  if (lightColLoc != -1) {
    SetShaderValue(shadowShader, lightColLoc, &dimmedSun, SHADER_UNIFORM_VEC4);
  }

  if (skyShader.id > 0) {
    int covLoc = GetShaderLocation(skyShader, "u_cloudCover");
    if (covLoc != -1)
      SetShaderValue(skyShader, covLoc, &weatherProf.cloudCover,
                     SHADER_UNIFORM_FLOAT);

    int falLoc = GetShaderLocation(skyShader, "u_cloudFalloff");
    if (falLoc != -1)
      SetShaderValue(skyShader, falLoc, &weatherProf.cloudFalloff,
                     SHADER_UNIFORM_FLOAT);

    int fogLoc = GetShaderLocation(skyShader, "u_fogDensity");
    if (fogLoc != -1)
      SetShaderValue(skyShader, fogLoc, &weatherProf.fogDensity,
                     SHADER_UNIFORM_FLOAT);

    int horLoc = GetShaderLocation(skyShader, "u_horizonColor");
    if (horLoc != -1) {
      Vector3 blendedHor =
          Vector3Multiply(dayNight.GetCurrentLighting().equatorColor,
                          weatherProf.horizonHazeTint);
      SetShaderValue(skyShader, horLoc, &blendedHor, SHADER_UNIFORM_VEC3);
    }
  }
}

void VolumetricCloudsScene::Draw3D() {
  Camera &cam = m_engine->GetCamera();

  // Render 3D Weather Precipitation
  m_engine->GetWeatherSystem().Draw3D(cam.position);
}

void VolumetricCloudsScene::Draw() {
  // Top-Left Diagnostics Overlay
  DrawRectangle(15, 15, 300, 85, Fade(BLACK, 0.6f));
  DrawRectangleLines(15, 15, 300, 85, LIME);

  // Time
  float t = m_engine->GetDayNightCycle().GetTime();
  int hours = (int)t;
  int minutes = (int)((t - (float)hours) * 60.0f);
  DrawText(TextFormat("TIME:    %02d:%02d", hours, minutes), 30, 25, 18, LIME);

  // Weather
  auto &weatherSys = m_engine->GetWeatherSystem();
  std::string weatherText =
      TextFormat("WEATHER: %s", weatherSys.GetCurrentWeatherName().c_str());
  if (weatherSys.IsTransitioning()) {
    weatherText +=
        TextFormat(" (%.0f%%)", weatherSys.GetTransitionProgress() * 100.0f);
  } else if (weatherSys.IsAutoWeatherEnabled()) {
    weatherText += " [AUTO]";
  }
  DrawText(weatherText.c_str(), 30, 52, 18, SKYBLUE);

  // Controls Help + Altitude
  DrawText(TextFormat("ALT: %.0fm | WASD: Move | Q/E: Down/Up | Shift: Boost | "
                      "Tab: Cursor | O/P: Time | K/L: Weather",
                      m_engine->GetCamera().position.y),
           20, GetScreenHeight() - 25, 16, RAYWHITE);
}

void VolumetricCloudsScene::Shutdown() {
  EnableCursor();
  if (m_modelLoaded) {
    UnloadModel(m_sampleModel);
    m_modelLoaded = false;
  }
}
