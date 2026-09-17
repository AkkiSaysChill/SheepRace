#include "game/MissionSelectScene.h"
#include "engine/Engine.h"
#include "engine/SceneManager.h"
#include "game/FlightSession.h"
#include "game/MissionDatabase.h"
#include <algorithm>

MissionSelectScene::MissionSelectScene()
    : m_engine(nullptr), m_selectedIndex(0), m_scrollOffset(0),
      m_fontLoaded(false) {}

MissionSelectScene::~MissionSelectScene() { Shutdown(); }

void MissionSelectScene::Initialize(Engine *engine) {
  m_engine = engine;

  // 1. Load mission assets into MissionDatabase
  auto &assetLoader = m_engine->GetAssetLoader();
  assetLoader.loadPackedFile("DATA/MISSIONS/missions.bin");
  auto missionBin = assetLoader.getAssetData("missions.xml");

  if (!missionBin.empty()) {
    MissionDatabase::GetInstance().LoadFromMemory(missionBin.data(),
                                                  missionBin.size());
  } else {
    MissionDatabase::GetInstance().LoadFromFile("DATA/MISSIONS/missions.xml");
  }

  // 2. Load custom font
  const char *fontPath = "DATA/FONTS/Audiowide-Regular.ttf";
  if (FileExists(fontPath)) {
    m_customFont = LoadFontEx(fontPath, 32, 0, 0);
    SetTextureFilter(m_customFont.texture, TEXTURE_FILTER_BILINEAR);
    m_fontLoaded = true;
  }

  // 3. Load or initialize player progress
  if (!m_progress.Load("player.save")) {
    EnsureProgressSynchronized();
    m_progress.Save("player.save");
  } else {
    EnsureProgressSynchronized();
  }
}

void MissionSelectScene::EnsureProgressSynchronized() {
  const auto &missionIds = MissionDatabase::GetInstance().GetAllMissionIds();
  if (m_progress.levels.size() < missionIds.size()) {
    size_t initialSize = m_progress.levels.size();
    for (size_t i = initialSize; i < missionIds.size(); ++i) {
      LevelStat stat;
      stat.unlocked = (i == 0); // First mission unlocked by default
      stat.stars = 0;
      stat.bestTime = 0.0f;
      m_progress.levels.push_back(stat);
    }
  }
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

  // Panel styling
  Color bgCol = isUnlocked
                    ? (active ? Color{25, 45, 65, 240} : Color{16, 26, 42, 210})
                    : Color{12, 14, 18, 220};
  Color borderCol = isUnlocked ? (active ? Color{100, 240, 255, 255}
                                         : Color{50, 110, 160, 160})
                               : Color{40, 45, 52, 180};

  DrawRectangleRounded(rect, 0.18f, 6, bgCol);
  DrawRectangleRoundedLines(rect, 0.18f, 6, borderCol);

  // Number badge
  const char *numStr = TextFormat("%02d", index + 1);
  DrawText(numStr, (int)rect.x + 14, (int)rect.y + 12, 18,
           isUnlocked ? YELLOW : DARKGRAY);

  if (isUnlocked) {
    DrawText(title.c_str(), (int)rect.x + 55, (int)rect.y + 12, 18, RAYWHITE);
    DrawText(TextFormat("AIRCRAFT: %s", plane.c_str()), (int)rect.x + 55,
             (int)rect.y + 34, 12, Color{100, 220, 255, 200});

    // Best Time & Stars
    if (bestTime > 0.0f) {
      int mins = (int)bestTime / 60;
      int secs = (int)bestTime % 60;
      DrawText(TextFormat("BEST: %02d:%02d", mins, secs),
               (int)(rect.x + rect.width - 130), (int)rect.y + 34, 12, GREEN);
    }

    // Star Badges
    for (int s = 0; s < 3; ++s) {
      Color starCol = (s < stars) ? GOLD : Fade(GRAY, 0.4f);
      DrawCircle((int)(rect.x + rect.width - 65 + (s * 18)), (int)rect.y + 20,
                 6.0f, starCol);
    }
  } else {
    DrawText("LOCKED OPERATION", (int)rect.x + 55, (int)rect.y + 14, 16,
             Fade(GRAY, 0.7f));
    DrawText("Complete previous mission to unlock", (int)rect.x + 55,
             (int)rect.y + 34, 12, DARKGRAY);
    DrawText("[ LOCKED ]", (int)(rect.x + rect.width - 95), (int)rect.y + 22,
             12, RED);
  }

  return isUnlocked && hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void MissionSelectScene::Update(float deltaTime) {
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

  // Scroll tracking
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
    m_engine->GetSceneManager().LoadScene("FlightScene");
  }
}

void MissionSelectScene::Draw() {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  DrawRectangleGradientV(0, 0, screenW, screenH, Color{10, 16, 26, 255},
                         Color{4, 6, 10, 255});

  // Header
  DrawText("CAMPAIGN THEATER: ARCHIPELAGO",
           screenW / 2 - MeasureText("CAMPAIGN THEATER: ARCHIPELAGO", 28) / 2,
           28, 28, LIME);
  DrawText(
      "Select operation to deploy. Locked missions require earlier clearance.",
      screenW / 2 - MeasureText("Select operation to deploy. Locked missions "
                                "require earlier clearance.",
                                13) /
                        2,
      64, 13, LIGHTGRAY);

  const auto &missionIds = MissionDatabase::GetInstance().GetAllMissionIds();
  float startY = 100.0f;
  float cardW = fminf((float)screenW - 120.0f, 680.0f);
  float cardH = 58.0f;
  float gap = 12.0f;
  float startX = (screenW - cardW) / 2.0f;

  int visibleCount = 5;
  for (int i = 0;
       i < visibleCount && (i + m_scrollOffset) < (int)missionIds.size(); ++i) {
    int idx = i + m_scrollOffset;
    Rectangle cardRect = {startX, startY + (i * (cardH + gap)), cardW, cardH};

    if (DrawMissionCard(idx, missionIds[idx], cardRect,
                        m_selectedIndex == idx)) {
      FlightSession::GetInstance().activeMissionId = missionIds[idx];
      m_engine->GetSceneManager().LoadScene("FlightScene");
      return;
    }
  }

  // Controls footer
  DrawRectangle(0, screenH - 36, screenW, 36, Color{8, 12, 18, 220});
  DrawText(
      "[W / S / ARROWS] Select  |  [ENTER] Deploy  |  [ESC] Main Menu",
      screenW / 2 -
          MeasureText(
              "[W / S / ARROWS] Select  |  [ENTER] Deploy  |  [ESC] Main Menu",
              13) /
              2,
      screenH - 24, 13, Color{140, 220, 255, 230});
}

void MissionSelectScene::Shutdown() {
  if (m_fontLoaded) {
    UnloadFont(m_customFont);
    m_fontLoaded = false;
  }
}
