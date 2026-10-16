#include "game/MissionSelectScene.h"
#include "engine/Engine.h"
#include "engine/SceneManager.h"
#include "game/FlightSession.h"
#include "game/MissionDatabase.h"
#include "game/Ocean.h"
#include "raymath.h"
#include <algorithm>

// Static assets for the 3D cinematic menu background
static Ocean *s_missionOcean = nullptr;
static float s_missionCamAngle = 0.0f;

MissionSelectScene::MissionSelectScene()
    : m_engine(nullptr), m_selectedIndex(0), m_scrollOffset(0),
      m_fontLoaded(false) {}

MissionSelectScene::~MissionSelectScene() { Shutdown(); }

void MissionSelectScene::Initialize(Engine *engine) {
  Scene3D::Initialize(engine);
  m_engine = engine;

  auto &assetLoader = m_engine->GetAssetLoader();
  assetLoader.loadPackedFile("DATA/MISSIONS/missions.bin");
  auto missionBin = assetLoader.getAssetData("missions.xml");

  if (!missionBin.empty()) {
    MissionDatabase::GetInstance().LoadFromMemory(missionBin.data(),
                                                  missionBin.size());
  } else {
    MissionDatabase::GetInstance().LoadFromFile("DATA/MISSIONS/missions.xml");
  }

  // Initialize 3D Environment for the Menu
  if (!s_missionOcean) {
    s_missionOcean = new Ocean();
    s_missionOcean->Initialize(m_engine, 260000.0f);
  }

  // Set to Bright Daytime
  m_engine->GetDayNightCycle().SetTime(12.0f);
  m_engine->GetDayNightCycle().SetTimeScale(0.02f);
  m_engine->GetWeatherSystem().SetWeather(WeatherPreset::CLEAR, 0.0f);

  // Setup the Engine Camera to look straight up into the clouds
  Camera &cam = m_engine->GetCamera();
  cam.position = Vector3{0.0f, 15.0f, 0.0f};
  cam.target = Vector3{0.0f, 500.0f, 0.0f};
  cam.up = Vector3{0.0f, 0.0f, -1.0f};
  cam.fovy = 85.0f;
  cam.projection = CAMERA_PERSPECTIVE;

  const char *fontPath = "DATA/FONTS/Audiowide-Regular.ttf";
  if (FileExists(fontPath)) {
    m_customFont = LoadFontEx(fontPath, 48, 0, 0);
    SetTextureFilter(m_customFont.texture, TEXTURE_FILTER_BILINEAR);
    m_fontLoaded = true;
  }

  if (!m_progress.Load("player.save")) {
    EnsureProgressSynchronized();
    m_progress.Save("player.save");
  } else {
    EnsureProgressSynchronized();
  }

  EnableCursor();
}

void MissionSelectScene::EnsureProgressSynchronized() {
  const auto &missionIds = MissionDatabase::GetInstance().GetAllMissionIds();
  if (m_progress.levels.size() < missionIds.size()) {
    size_t initialSize = m_progress.levels.size();
    for (size_t i = initialSize; i < missionIds.size(); ++i) {
      LevelStat stat;
      stat.unlocked = (i == 0);
      stat.stars = 0;
      stat.bestTime = 0.0f;
      m_progress.levels.push_back(stat);
    }
  }
}

static void DrawGlassPanel(Rectangle rect, float roundness = 0.25f,
                           Color tint = Color{18, 28, 48, 160},
                           bool highlight = false) {
  DrawRectangleRounded(
      Rectangle{rect.x + 4.0f, rect.y + 5.0f, rect.width, rect.height},
      roundness, 8, Fade(BLACK, 0.45f));
  DrawRectangleRounded(rect, roundness, 8, tint);

  Color outline = highlight ? Color{100, 240, 255, 255}
                            : Fade(Color{130, 220, 255, 230}, 0.3f);
  DrawRectangleRoundedLines(rect, roundness, 8, outline);

  Rectangle glossRect = {rect.x + 5.0f, rect.y + 3.0f, rect.width - 10.0f,
                         rect.height * 0.30f};
  DrawRectangleRounded(glossRect, 0.5f, 6,
                       Fade(WHITE, highlight ? 0.15f : 0.05f));
}

void MissionSelectScene::DrawUIString(const char *text, float posX, float posY,
                                      float fontSize, Color color) {
  if (m_fontLoaded) {
    DrawTextEx(m_customFont, text, Vector2{posX + 2.0f, posY + 2.0f}, fontSize,
               1.0f, Fade(BLACK, 0.75f));
    DrawTextEx(m_customFont, text, Vector2{posX, posY}, fontSize, 1.0f, color);
  } else {
    DrawText(text, (int)posX, (int)posY, (int)fontSize, color);
  }
}

float MissionSelectScene::MeasureUIString(const char *text, float fontSize) {
  if (m_fontLoaded) {
    return MeasureTextEx(m_customFont, text, fontSize, 1.0f).x;
  }
  return (float)MeasureText(text, (int)fontSize);
}

bool MissionSelectScene::DrawMissionCard(int index,
                                         const std::string &missionId,
                                         Rectangle rect, bool isSelected) {
  Vector2 mousePos = GetMousePosition();
  bool hovered = CheckCollisionPointRec(mousePos, rect);
  bool active = isSelected || hovered;

  bool isUnlocked = (index < (int)m_progress.levels.size()) &&
                    m_progress.levels[index].unlocked;
  int stars = (index < (int)m_progress.levels.size())
                  ? m_progress.levels[index].stars
                  : 0;
  float bestTime = (index < (int)m_progress.levels.size())
                       ? m_progress.levels[index].bestTime
                       : 0.0f;

  const MissionData *mData =
      MissionDatabase::GetInstance().GetMission(missionId);
  std::string title = mData ? mData->title : missionId;
  std::string plane = mData ? mData->requiredPlaneId : "trainer";

  Color bgCol = isUnlocked
                    ? (active ? Color{25, 55, 85, 230} : Color{14, 26, 42, 190})
                    : Color{12, 14, 18, 220};

  DrawGlassPanel(rect, 0.18f, bgCol, active);

  const char *numStr = TextFormat("%02d", index + 1);
  DrawUIString(numStr, rect.x + 14.0f, rect.y + 16.0f, 22.0f,
               isUnlocked ? Color{255, 215, 0, 255} : DARKGRAY);

  if (isUnlocked) {
    DrawUIString(title.c_str(), rect.x + 60.0f, rect.y + 14.0f, 18.0f,
                 RAYWHITE);
    DrawUIString(TextFormat("AIRCRAFT: %s", plane.c_str()), rect.x + 60.0f,
                 rect.y + 36.0f, 12.0f, Color{100, 220, 255, 220});

    if (bestTime > 0.0f) {
      int mins = (int)bestTime / 60;
      int secs = (int)bestTime % 60;
      DrawUIString(TextFormat("BEST: %02d:%02d", mins, secs),
                   rect.x + rect.width - 145.0f, rect.y + 36.0f, 12.0f,
                   Color{100, 255, 130, 255});
    }

    for (int s = 0; s < 3; ++s) {
      Color starCol = (s < stars) ? Color{255, 215, 0, 255} : Fade(GRAY, 0.4f);
      DrawCircle((int)(rect.x + rect.width - 70 + (s * 20)), (int)rect.y + 24,
                 7.0f, starCol);
      DrawCircleLines((int)(rect.x + rect.width - 70 + (s * 20)),
                      (int)rect.y + 24, 7.0f, Fade(BLACK, 0.5f));
    }
  } else {
    DrawUIString("LOCKED OPERATION", rect.x + 60.0f, rect.y + 16.0f, 16.0f,
                 Fade(GRAY, 0.8f));
    DrawUIString("Complete previous mission to unlock", rect.x + 60.0f,
                 rect.y + 36.0f, 12.0f, DARKGRAY);
    DrawUIString("[ LOCKED ]", rect.x + rect.width - 105.0f, rect.y + 24.0f,
                 13.0f, Color{255, 70, 70, 255});
  }

  return isUnlocked && hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void MissionSelectScene::Update(float deltaTime) {
  // Rotate target slowly so the clouds look dynamic while looking straight up
  s_missionCamAngle -= deltaTime * 0.15f;
  Camera &cam = m_engine->GetCamera();
  cam.target = Vector3{cosf(s_missionCamAngle) * 50.0f, 500.0f,
                       sinf(s_missionCamAngle) * 50.0f};

  // Update Day/Night and Weather Shaders
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
  s_missionOcean->Update(deltaTime, cam, skySun);

  // Menu Input Logic
  const auto &missionIds = MissionDatabase::GetInstance().GetAllMissionIds();
  int count = (int)missionIds.size();
  if (count == 0)
    return;

  if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
    m_selectedIndex = (m_selectedIndex + 1) % count;
  }
  if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
    m_selectedIndex = (m_selectedIndex + count - 1) % count;
  }
  if (IsKeyPressed(KEY_ESCAPE)) {
    m_engine->GetSceneManager().LoadScene("MainMenuScene");
    return;
  }

  if (m_selectedIndex < m_scrollOffset) {
    m_scrollOffset = m_selectedIndex;
  } else if (m_selectedIndex >= m_scrollOffset + 5) {
    m_scrollOffset = m_selectedIndex - 4;
  }

  bool enter = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
  bool isUnlocked = (m_selectedIndex < (int)m_progress.levels.size()) &&
                    m_progress.levels[m_selectedIndex].unlocked;

  if (enter && isUnlocked) {
    FlightSession::GetInstance().activeMissionId = missionIds[m_selectedIndex];
    m_engine->GetSceneManager().LoadScene("LoadingScene");
  }
}

void MissionSelectScene::Draw3D() {
  // Let the engine draw the 3D objects properly!
  Camera &cam = m_engine->GetCamera();
  s_missionOcean->Draw();
  m_engine->GetWeatherSystem().Draw3D(cam.position);
}

void MissionSelectScene::Draw() {
  // This function is purely for 2D UI over the 3D Engine output
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  // Dark vignette overlay to ensure mission text is readable against bright sky
  DrawRectangleGradientV(0, 0, screenW, screenH, Fade(BLACK, 0.4f),
                         Fade(BLACK, 0.85f));

  // --- Render UI ---
  const char *header = "CAMPAIGN THEATER: ARCHIPELAGO";
  DrawUIString(header, screenW / 2.0f - MeasureUIString(header, 26.0f) / 2.0f,
               28.0f, 26.0f, Color{100, 245, 140, 255});

  const char *subHeader =
      "Select operation to deploy. Locked missions require earlier clearance.";
  DrawUIString(subHeader,
               screenW / 2.0f - MeasureUIString(subHeader, 13.0f) / 2.0f, 62.0f,
               13.0f, Color{200, 220, 240, 200});

  const auto &missionIds = MissionDatabase::GetInstance().GetAllMissionIds();
  float startY = 110.0f;
  float cardW = fminf((float)screenW - 120.0f, 720.0f);
  float cardH = 64.0f;
  float gap = 16.0f;
  float startX = (screenW - cardW) / 2.0f;

  int visibleCount = 5;
  for (int i = 0;
       i < visibleCount && (i + m_scrollOffset) < (int)missionIds.size(); ++i) {
    int idx = i + m_scrollOffset;
    Rectangle cardRect = {startX, startY + (i * (cardH + gap)), cardW, cardH};

    if (DrawMissionCard(idx, missionIds[idx], cardRect,
                        m_selectedIndex == idx)) {
      FlightSession::GetInstance().activeMissionId = missionIds[idx];
      m_engine->GetSceneManager().LoadScene("LoadingScene");
      return;
    }
  }

  // Controls footer panel
  Rectangle footerRect = {-10.0f, (float)screenH - 46.0f,
                          (float)screenW + 20.0f, 56.0f};
  DrawGlassPanel(footerRect, 0.0f, Color{10, 16, 26, 230}, false);

  const char *footerText =
      "[W / S / ARROWS] Select   |   [ENTER] Deploy   |   [ESC] Main Menu";
  DrawUIString(footerText,
               screenW / 2.0f - MeasureUIString(footerText, 14.0f) / 2.0f,
               screenH - 30.0f, 14.0f, Color{140, 220, 255, 230});
}

void MissionSelectScene::Shutdown() {
  if (s_missionOcean) {
    s_missionOcean->Unload();
    delete s_missionOcean;
    s_missionOcean = nullptr;
  }
  if (m_fontLoaded) {
    UnloadFont(m_customFont);
    m_fontLoaded = false;
  }
}
