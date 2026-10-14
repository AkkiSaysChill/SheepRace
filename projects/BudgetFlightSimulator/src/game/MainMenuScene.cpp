#include "game/MainMenuScene.h"
#include "engine/Engine.h"
#include "engine/SceneManager.h"
#include "game/AircraftDatabase.h"
#include "game/FlightSession.h"

MainMenuScene::MainMenuScene() : m_engine(nullptr) {}
MainMenuScene::~MainMenuScene() {}

void MainMenuScene::Initialize(Engine *engine) {
  Scene2D::Initialize(engine);
  m_engine = engine;

  auto &assetLoader = m_engine->GetAssetLoader();
  assetLoader.loadPackedFile("shaders.bin");
  assetLoader.loadPackedFile("planes.woosh");

  auto xmlData = assetLoader.getAssetData("aircraft.xml");
  if (!xmlData.empty()) {
    AircraftDatabase::GetInstance().LoadFromMemory(xmlData.data(),
                                                   xmlData.size());
  } else {
    AircraftDatabase::GetInstance().LoadFromFile("assets/data/aircraft.xml");
  }
}

bool MainMenuScene::DrawButton(const char *text, Rectangle rect,
                               bool isSelected) {
  Vector2 mousePos = GetMousePosition();
  bool hovered = CheckCollisionPointRec(mousePos, rect);
  bool active = isSelected || hovered;

  Color bgColor = active ? Fade(LIME, 0.25f) : Fade(BLACK, 0.6f);
  Color borderColor = active ? LIME : DARKGRAY;
  Color textColor = active ? YELLOW : RAYWHITE;

  DrawRectangleRec(rect, bgColor);
  DrawRectangleLinesEx(rect, 2.0f, borderColor);

  int fontSize = 22;
  int textWidth = MeasureText(text, fontSize);
  DrawText(text, (int)(rect.x + (rect.width - textWidth) / 2),
           (int)(rect.y + (rect.height - fontSize) / 2), fontSize, textColor);

  return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void MainMenuScene::Update(float deltaTime) {
  if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
    m_selectedOption = (m_selectedOption + 1) % m_totalOptions;
  }
  if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
    m_selectedOption = (m_selectedOption + m_totalOptions - 1) % m_totalOptions;
  }

  bool selectTriggered = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

  // Option 0: Campaign Mission Select
  if (selectTriggered && m_selectedOption == 0) {
    m_engine->GetSceneManager().LoadScene("MissionSelectScene");
  }

  // Option 1: True Free Roam Mode (No active mission ID)
  if (selectTriggered && m_selectedOption == 1) {
    FlightSession::GetInstance().activeMissionId = "";
    m_engine->GetSceneManager().LoadScene("FlightScene");
  }

  // Option 2: Exit to Desktop
  if (selectTriggered && m_selectedOption == 2) {
    m_engine->RequestExit();
  }
}

void MainMenuScene::Draw() {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  DrawRectangleGradientV(0, 0, screenW, screenH, Color{15, 20, 30, 255},
                         Color{5, 8, 12, 255});

  for (int y = screenH / 2; y < screenH; y += 40) {
    DrawLine(0, y, screenW, y, Fade(LIME, 0.08f));
  }

  const char *title = "BUDGET FLIGHT SIMULATOR";
  DrawText(title, screenW / 2 - MeasureText(title, 36) / 2, screenH / 5, 36,
           LIME);

  float btnWidth = 320.0f;
  float btnHeight = 46.0f;
  float startY = screenH * 0.42f;
  float gap = 18.0f;

  Rectangle btnCampaign = {(screenW - btnWidth) / 2.0f, startY, btnWidth,
                           btnHeight};
  Rectangle btnFreeRoam = {(screenW - btnWidth) / 2.0f,
                           startY + (btnHeight + gap), btnWidth, btnHeight};
  Rectangle btnExit = {(screenW - btnWidth) / 2.0f,
                       startY + (btnHeight + gap) * 2, btnWidth, btnHeight};

  if (DrawButton("CAMPAIGN MISSIONS", btnCampaign, m_selectedOption == 0)) {
    m_engine->GetSceneManager().LoadScene("MissionSelectScene");
  }
  if (DrawButton("FREE ROAM", btnFreeRoam, m_selectedOption == 1)) {
    FlightSession::GetInstance().activeMissionId = "";
    m_engine->GetSceneManager().LoadScene("FlightScene");
  }
  if (DrawButton("EXIT TO DESKTOP", btnExit, m_selectedOption == 2)) {
    m_engine->RequestExit();
  }

  DrawText("Use [UP / DOWN] and [ENTER] or Mouse to navigate", 20, screenH - 30,
           14, DARKGRAY);
}

void MainMenuScene::Shutdown() {}
