#include "game/MainMenuScene.h"
#include "engine/Engine.h"
#include "engine/SceneManager.h"
#include "game/AircraftDatabase.h"
#include "game/FlightSession.h"
#include "game/Ocean.h"
#include "raymath.h"
#include "rlgl.h"

// Only this remains static so the intro flyby only plays on the first launch
static bool s_gameJustLaunched = true;

MainMenuScene::MainMenuScene() : m_engine(nullptr), m_ocean(nullptr) {}
MainMenuScene::~MainMenuScene() { Shutdown(); }

void MainMenuScene::Initialize(Engine *engine) {
  Scene3D::Initialize(engine);
  m_engine = engine;

  if (s_gameJustLaunched) {
    m_introTimer = 0.0f;
    m_menuAlpha = 0.0f;
    m_screenShake = 0.0f;
    m_playedFlyby = false;

    m_planePos = Vector3{180.0f, 130.0f, 180.0f};
    s_gameJustLaunched = false;
  } else {
    m_introTimer = 999.0f;
    m_menuAlpha = 1.0f;
    m_screenShake = 0.0f;
    m_playedFlyby = true;
  }

  auto &assetLoader = m_engine->GetAssetLoader();
  assetLoader.loadPackedFile("shaders.bin");
  assetLoader.loadPackedFile("planes.woosh");
  assetLoader.loadPackedFile("DATA/SFX/sfx.bin");

  auto xmlData = assetLoader.getAssetData("aircraft.xml");
  if (!xmlData.empty()) {
    AircraftDatabase::GetInstance().LoadFromMemory(xmlData.data(),
                                                   xmlData.size());
  } else {
    AircraftDatabase::GetInstance().LoadFromFile("assets/data/aircraft.xml");
  }

  AircraftStats cargoStats;
  if (AircraftDatabase::GetInstance().GetStats("cargo", cargoStats)) {
    auto cargoData = assetLoader.getAssetData(cargoStats.modelFile);
    if (!cargoData.empty()) {
      m_cargoModel = assetLoader.LoadModelFromAssetData(cargoData, ".glb");
      Shader shadowShader = m_engine->GetRenderer().GetShadowShader();
      for (int i = 0; i < m_cargoModel.materialCount; i++) {
        m_cargoModel.materials[i].shader = shadowShader;
      }
      m_cargoLoaded = true;
    }
  }

  auto sfxData = assetLoader.getAssetData("cargo_engine.wav");
  if (!sfxData.empty()) {
    Wave wave = LoadWaveFromMemory(
        ".wav", (const unsigned char *)sfxData.data(), (int)sfxData.size());
    m_sndFlyby = LoadSoundFromWave(wave);
    UnloadWave(wave);
  }

  if (!m_ocean) {
    m_ocean = new Ocean();
    m_ocean->Initialize(m_engine, 260000.0f);
  }

  m_engine->GetDayNightCycle().SetTime(12.0f);
  m_engine->GetDayNightCycle().SetTimeScale(0.015f);
  m_engine->GetWeatherSystem().SetWeather(WeatherPreset::PARTLY_CLOUDY, 0.0f);

  if (!m_mainFontLoaded) {
    const char *fontPath = "DATA/FONTS/Audiowide-Regular.ttf";
    if (FileExists(fontPath)) {
      m_mainFont = LoadFontEx(fontPath, 48, 0, 0);
      SetTextureFilter(m_mainFont.texture, TEXTURE_FILTER_BILINEAR);
      m_mainFontLoaded = true;
    }
  }

  EnableCursor();
}

static void DrawGlassPanel(Rectangle rect, float roundness = 0.25f,
                           Color tint = Color{18, 28, 48, 160},
                           bool highlight = false, float alphaMulti = 1.0f) {
  DrawRectangleRounded(
      Rectangle{rect.x + 4.0f, rect.y + 5.0f, rect.width, rect.height},
      roundness, 8, Fade(BLACK, 0.45f * alphaMulti));
  DrawRectangleRounded(rect, roundness, 8,
                       Fade(tint, (tint.a / 255.0f) * alphaMulti));
  Color outline =
      highlight ? Color{100, 240, 255, 255} : Color{130, 220, 255, 230};
  DrawRectangleRoundedLines(
      rect, roundness, 8,
      Fade(outline, (highlight ? 1.0f : 0.5f) * alphaMulti));
  Rectangle glossRect = {rect.x + 5.0f, rect.y + 3.0f, rect.width - 10.0f,
                         rect.height * 0.30f};
  DrawRectangleRounded(glossRect, 0.5f, 6,
                       Fade(WHITE, (highlight ? 0.18f : 0.08f) * alphaMulti));
}

void MainMenuScene::DrawUIStringMenu(const char *text, float posX, float posY,
                                     float fontSize, Color color,
                                     float alphaMulti) {
  if (m_mainFontLoaded) {
    DrawTextEx(m_mainFont, text, Vector2{posX + 2.0f, posY + 2.0f}, fontSize,
               1.0f, Fade(BLACK, 0.75f * alphaMulti));
    DrawTextEx(m_mainFont, text, Vector2{posX, posY}, fontSize, 1.0f,
               Fade(color, (color.a / 255.0f) * alphaMulti));
  } else {
    DrawText(text, (int)posX, (int)posY, (int)fontSize,
             Fade(color, (color.a / 255.0f) * alphaMulti));
  }
}

float MainMenuScene::MeasureUIStringMenu(const char *text, float fontSize) {
  if (m_mainFontLoaded) {
    return MeasureTextEx(m_mainFont, text, fontSize, 1.0f).x;
  }
  return (float)MeasureText(text, (int)fontSize);
}

bool MainMenuScene::DrawButton(const char *text, Rectangle rect,
                               bool isSelected, float alphaMulti) {
  Vector2 mousePos = GetMousePosition();
  bool hovered = CheckCollisionPointRec(mousePos, rect);

  if (alphaMulti < 0.8f)
    hovered = false;
  bool active = isSelected || hovered;

  Color bgColor = active ? Color{35, 65, 95, 210} : Color{14, 26, 42, 180};
  Color textColor = active ? Color{255, 215, 0, 255} : RAYWHITE;

  DrawGlassPanel(rect, 0.3f, bgColor, active, alphaMulti);

  float fontSize = 20.0f;
  float textWidth = MeasureUIStringMenu(text, fontSize);
  DrawUIStringMenu(text, rect.x + (rect.width - textWidth) / 2.0f,
                   rect.y + (rect.height - fontSize) / 2.0f, fontSize,
                   textColor, alphaMulti);

  return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
         alphaMulti >= 0.8f;
}

void MainMenuScene::Update(float deltaTime) {
  m_introTimer += deltaTime;

  Camera &cam = m_engine->GetCamera();
  cam.position = Vector3{0.0f, 15.0f, 0.0f};
  cam.target = Vector3{0.0f, 500.0f, 0.0f};
  cam.up = Vector3{0.0f, 0.0f, -1.0f};
  cam.fovy = 80.0f;
  cam.projection = CAMERA_PERSPECTIVE;

  if (m_introTimer < 25.0f) {
    float speed = 65.0f;
    m_planePos.x -= speed * 0.7071f * deltaTime;
    m_planePos.z -= speed * 0.7071f * deltaTime;

    float distToCam = Vector2Distance(Vector2{m_planePos.x, m_planePos.z},
                                      Vector2{cam.position.x, cam.position.z});
    if (distToCam < 150.0f) {
      m_screenShake = (150.0f - distToCam) / 150.0f * 1.5f;
    } else {
      m_screenShake -= deltaTime * 2.0f;
      if (m_screenShake < 0.0f)
        m_screenShake = 0.0f;
    }

    if (m_screenShake > 0.01f) {
      cam.target.x +=
          (((float)rand() / (float)RAND_MAX) - 0.5f) * m_screenShake * 5.0f;
      cam.target.z +=
          (((float)rand() / (float)RAND_MAX) - 0.5f) * m_screenShake * 5.0f;
    }

    if (m_introTimer > 0.1f && m_planePos.x > -250.0f) {
      if (!IsSoundPlaying(m_sndFlyby)) {
        SetSoundPitch(m_sndFlyby, 0.40f);
        PlaySound(m_sndFlyby);
      }
      float vol = Clamp(1.2f - (distToCam / 220.0f), 0.0f, 1.0f);
      SetSoundVolume(m_sndFlyby, vol);
    } else {
      if (IsSoundPlaying(m_sndFlyby))
        StopSound(m_sndFlyby);
    }

    if (m_introTimer > 3.0f) {
      m_menuAlpha += deltaTime * 0.75f;
      if (m_menuAlpha > 1.0f)
        m_menuAlpha = 1.0f;
    }
  }

  auto &weatherSys = m_engine->GetWeatherSystem();
  weatherSys.Update(deltaTime, cam.position, Vector3Zero());

  auto &dayNight = m_engine->GetDayNightCycle();
  dayNight.Update(deltaTime);

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

  Vector3 skySun = Vector3Negate(dayNight.GetCurrentLighting().lightDir);
  if (m_ocean)
    m_ocean->Update(deltaTime, cam, skySun);

  if (m_menuAlpha > 0.5f) {
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
      m_selectedOption = (m_selectedOption + 1) % m_totalOptions;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
      m_selectedOption =
          (m_selectedOption + m_totalOptions - 1) % m_totalOptions;

    bool selectTriggered = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

    if (selectTriggered && m_selectedOption == 0)
      m_engine->GetSceneManager().LoadScene("MissionSelectScene");
    if (selectTriggered && m_selectedOption == 1) {
      FlightSession::GetInstance().activeMissionId = "";
      m_engine->GetSceneManager().LoadScene("LoadingScene");
    }
    if (selectTriggered && m_selectedOption == 2)
      m_engine->RequestExit();
  }
}

void MainMenuScene::Draw3D() {
  Camera &cam = m_engine->GetCamera();
  if (m_ocean)
    m_ocean->Draw();

  if (m_cargoLoaded && m_introTimer < 25.0f && m_planePos.x > -300.0f) {
    float animProg = Clamp(m_introTimer / 6.0f, 0.0f, 1.0f);
    float pitch = Lerp(10.0f, 0.0f, animProg);
    float roll = Lerp(25.0f, 0.0f, animProg);

    m_cargoModel.transform = QuaternionToMatrix(
        QuaternionFromEuler(pitch * DEG2RAD, 45.0f * DEG2RAD, roll * DEG2RAD));
    DrawModel(m_cargoModel, m_planePos, 4.0f, WHITE);
  }

  m_engine->GetWeatherSystem().Draw3D(cam.position);
}

void MainMenuScene::Draw() {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  if (m_menuAlpha > 0.01f) {
    DrawRectangleGradientV(0, 0, screenW, screenH,
                           Fade(BLACK, 0.1f * m_menuAlpha),
                           Fade(BLACK, 0.8f * m_menuAlpha));

    const char *title = "BUDGET FLIGHT SIMULATOR";
    float titleW = MeasureUIStringMenu(title, 42.0f);

    Rectangle titlePanel = {screenW / 2.0f - titleW / 2.0f - 30.0f,
                            screenH * 0.18f - 20.0f, titleW + 60.0f, 85.0f};
    DrawGlassPanel(titlePanel, 0.4f, Color{10, 15, 25, 140}, false,
                   m_menuAlpha);
    DrawUIStringMenu(title, screenW / 2.0f - titleW / 2.0f, screenH * 0.18f,
                     42.0f, Color{120, 240, 255, 255}, m_menuAlpha);

    float btnWidth = 340.0f;
    float btnHeight = 52.0f;
    float startY = screenH * 0.45f;
    float gap = 24.0f;

    Rectangle btnCampaign = {(screenW - btnWidth) / 2.0f, startY, btnWidth,
                             btnHeight};
    Rectangle btnFreeRoam = {(screenW - btnWidth) / 2.0f,
                             startY + (btnHeight + gap), btnWidth, btnHeight};
    Rectangle btnExit = {(screenW - btnWidth) / 2.0f,
                         startY + (btnHeight + gap) * 2, btnWidth, btnHeight};

    if (DrawButton("CAMPAIGN MISSIONS", btnCampaign, m_selectedOption == 0,
                   m_menuAlpha)) {
      m_engine->GetSceneManager().LoadScene("MissionSelectScene");
    }
    if (DrawButton("FREE ROAM", btnFreeRoam, m_selectedOption == 1,
                   m_menuAlpha)) {
      FlightSession::GetInstance().activeMissionId = "";
      m_engine->GetSceneManager().LoadScene("LoadingScene");
    }
    if (DrawButton("EXIT TO DESKTOP", btnExit, m_selectedOption == 2,
                   m_menuAlpha)) {
      m_engine->RequestExit();
    }

    const char *footer = "Use [UP / DOWN] and [ENTER] or Mouse to navigate";
    DrawUIStringMenu(
        footer, screenW / 2.0f - MeasureUIStringMenu(footer, 14.0f) / 2.0f,
        screenH - 40.0f, 14.0f, Color{200, 200, 200, 180}, m_menuAlpha);
  }
}

void MainMenuScene::Shutdown() {
  if (IsSoundValid(m_sndFlyby)) {
    StopSound(m_sndFlyby);
    UnloadSound(m_sndFlyby);
    m_sndFlyby = {};
  }
  if (m_cargoLoaded) {
    UnloadModel(m_cargoModel);
    m_cargoLoaded = false;
  }
  if (m_ocean) {
    m_ocean->Unload();
    delete m_ocean;
    m_ocean = nullptr;
  }
  if (m_mainFontLoaded) {
    UnloadFont(m_mainFont);
    m_mainFontLoaded = false;
  }
}
