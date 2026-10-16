#include "game/MainMenuScene.h"
#include "engine/Engine.h"
#include "engine/SceneManager.h"
#include "game/AircraftDatabase.h"
#include "game/FlightSession.h"
#include "game/Ocean.h"
#include "raymath.h"
#include "rlgl.h"

// Static assets for the 3D cinematic menu background
static Ocean *s_mainOcean = nullptr;
static Font s_mainFont = {0};
static bool s_mainFontLoaded = false;
static bool s_gameJustLaunched = true; // Tracks if this is the initial launch

MainMenuScene::MainMenuScene() : m_engine(nullptr) {}
MainMenuScene::~MainMenuScene() { Shutdown(); }

void MainMenuScene::Initialize(Engine *engine) {
  Scene3D::Initialize(engine);
  m_engine = engine;

  if (s_gameJustLaunched) {
    m_introTimer = 0.0f;
    m_menuAlpha = 0.0f;
    m_screenShake = 0.0f;
    m_playedFlyby = false;

    // Spawn heavy plane closer to the view frustum edge (bottom right)
    m_planePos = Vector3{180.0f, 130.0f, 180.0f};

    // Crucial fix: Flag it false IMMEDIATELY so it never replays if you skip
    // scenes early
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

  // Load Cargo model for the flyover sequence
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

  // Load the heavy cargo engine sound effect instead of the explosion
  auto sfxData = assetLoader.getAssetData("cargo_engine.wav");
  if (!sfxData.empty()) {
    Wave wave = LoadWaveFromMemory(
        ".wav", (const unsigned char *)sfxData.data(), (int)sfxData.size());
    m_sndFlyby = LoadSoundFromWave(wave);
    UnloadWave(wave);
  }

  // Initialize 3D Environment for the Menu
  if (!s_mainOcean) {
    s_mainOcean = new Ocean();
    s_mainOcean->Initialize(m_engine, 260000.0f);
  }

  // Set to Bright Daytime
  m_engine->GetDayNightCycle().SetTime(12.0f);
  m_engine->GetDayNightCycle().SetTimeScale(0.015f); // Very slow sun crawl
  m_engine->GetWeatherSystem().SetWeather(WeatherPreset::PARTLY_CLOUDY, 0.0f);

  if (!s_mainFontLoaded) {
    const char *fontPath = "DATA/FONTS/Audiowide-Regular.ttf";
    if (FileExists(fontPath)) {
      s_mainFont = LoadFontEx(fontPath, 48, 0, 0);
      SetTextureFilter(s_mainFont.texture, TEXTURE_FILTER_BILINEAR);
      s_mainFontLoaded = true;
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

static void DrawUIStringMenu(const char *text, float posX, float posY,
                             float fontSize, Color color,
                             float alphaMulti = 1.0f) {
  if (s_mainFontLoaded) {
    DrawTextEx(s_mainFont, text, Vector2{posX + 2.0f, posY + 2.0f}, fontSize,
               1.0f, Fade(BLACK, 0.75f * alphaMulti));
    DrawTextEx(s_mainFont, text, Vector2{posX, posY}, fontSize, 1.0f,
               Fade(color, (color.a / 255.0f) * alphaMulti));
  } else {
    DrawText(text, (int)posX, (int)posY, (int)fontSize,
             Fade(color, (color.a / 255.0f) * alphaMulti));
  }
}

static float MeasureUIStringMenu(const char *text, float fontSize) {
  if (s_mainFontLoaded) {
    return MeasureTextEx(s_mainFont, text, fontSize, 1.0f).x;
  }
  return (float)MeasureText(text, (int)fontSize);
}

bool MainMenuScene::DrawButton(const char *text, Rectangle rect,
                               bool isSelected, float alphaMulti) {
  Vector2 mousePos = GetMousePosition();
  bool hovered = CheckCollisionPointRec(mousePos, rect);

  // Prevent phantom clicks while the menu is invisible or fading in
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

  // Configure the built-in Engine Camera to point straight UP at the sky
  Camera &cam = m_engine->GetCamera();
  cam.position = Vector3{0.0f, 15.0f, 0.0f};
  cam.target = Vector3{0.0f, 500.0f, 0.0f};
  cam.up = Vector3{0.0f, 0.0f, -1.0f};
  cam.fovy = 80.0f;
  cam.projection = CAMERA_PERSPECTIVE;

  if (m_introTimer < 25.0f) { // Increased boundary to let the plane finish its
                              // animation safely
    // 1. Cinematic Heavy Plane Flyover (Bottom right to top left)
    float speed =
        65.0f; // Adjusted speed to cross frame perfectly in ~7 seconds
    m_planePos.x -= speed * 0.7071f * deltaTime; // Move diagonally -X
    m_planePos.z -= speed * 0.7071f * deltaTime; // Move diagonally -Z

    // Screen shake as it flies overhead (Significantly reduced intensity)
    float distToCam = Vector2Distance(Vector2{m_planePos.x, m_planePos.z},
                                      Vector2{cam.position.x, cam.position.z});
    if (distToCam < 150.0f) {
      m_screenShake = (150.0f - distToCam) / 150.0f * 1.5f;
    } else {
      m_screenShake -= deltaTime * 2.0f;
      if (m_screenShake < 0.0f)
        m_screenShake = 0.0f;
    }

    // Apply Shake to Camera Target
    if (m_screenShake > 0.01f) {
      cam.target.x +=
          (((float)rand() / (float)RAND_MAX) - 0.5f) * m_screenShake * 5.0f;
      cam.target.z +=
          (((float)rand() / (float)RAND_MAX) - 0.5f) * m_screenShake * 5.0f;
    }

    // Audio trigger (Continuous looping and volume fading based on distance)
    if (m_introTimer > 0.1f && m_planePos.x > -250.0f) {
      if (!IsSoundPlaying(m_sndFlyby)) {
        SetSoundPitch(m_sndFlyby, 0.40f); // Deep drone
        PlaySound(m_sndFlyby);
      }

      // Calculate realistic fade-out as the plane gets further away
      float vol = Clamp(1.2f - (distToCam / 220.0f), 0.0f, 1.0f);
      SetSoundVolume(m_sndFlyby, vol);
    } else {
      if (IsSoundPlaying(m_sndFlyby)) {
        StopSound(m_sndFlyby); // Cut the sound smoothly when it's out of bounds
      }
    }

    // 2. Menu Fade Logic (Triggers as plane passes the center of the screen)
    if (m_introTimer > 3.0f) {
      m_menuAlpha += deltaTime * 0.75f;
      if (m_menuAlpha > 1.0f)
        m_menuAlpha = 1.0f;
    }
  }

  // 3. Update Day/Night and Weather Shaders
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
  s_mainOcean->Update(deltaTime, cam, skySun);

  // 4. Input Logic (Only allowed once menu is visible)
  if (m_menuAlpha > 0.5f) {
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
      m_selectedOption = (m_selectedOption + 1) % m_totalOptions;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
      m_selectedOption =
          (m_selectedOption + m_totalOptions - 1) % m_totalOptions;
    }

    bool selectTriggered = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

    if (selectTriggered && m_selectedOption == 0) {
      m_engine->GetSceneManager().LoadScene("MissionSelectScene");
    }

    if (selectTriggered && m_selectedOption == 1) {
      FlightSession::GetInstance().activeMissionId = "";
      m_engine->GetSceneManager().LoadScene("LoadingScene");
    }

    if (selectTriggered && m_selectedOption == 2) {
      m_engine->RequestExit();
    }
  }
}

void MainMenuScene::Draw3D() {
  Camera &cam = m_engine->GetCamera();
  s_mainOcean->Draw();

  // Ensure the model draws until it is physically off the screen (X < -300)
  if (m_cargoLoaded && m_introTimer < 25.0f && m_planePos.x > -300.0f) {
    // Simulate heavy stabilizing pitch and roll over time
    float animProg = Clamp(m_introTimer / 6.0f, 0.0f, 1.0f);
    float pitch = Lerp(10.0f, 0.0f, animProg); // Starts pitched up, levels out
    float roll = Lerp(25.0f, 0.0f, animProg); // Banks into the turn, levels out

    // Yaw 45 points it diagonally forward toward top-left (-X, -Z)
    m_cargoModel.transform = QuaternionToMatrix(
        QuaternionFromEuler(pitch * DEG2RAD, 45.0f * DEG2RAD, roll * DEG2RAD));
    DrawModel(m_cargoModel, m_planePos, 4.0f, WHITE); // Render massive
  }

  m_engine->GetWeatherSystem().Draw3D(cam.position);
}

void MainMenuScene::Draw() {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  // --- 2. Render Fading UI Elements ---
  if (m_menuAlpha > 0.01f) {
    // Dark vignette overlay to make text pop against the bright sky
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
  if (s_mainOcean) {
    s_mainOcean->Unload();
    delete s_mainOcean;
    s_mainOcean = nullptr;
  }
  if (s_mainFontLoaded) {
    UnloadFont(s_mainFont);
    s_mainFontLoaded = false;
  }
}
