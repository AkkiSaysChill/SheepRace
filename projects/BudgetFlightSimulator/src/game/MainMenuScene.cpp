#include "game/MainMenuScene.h"
#include "engine/Engine.h"
#include "engine/SceneManager.h"
#include "game/AircraftDatabase.h"

MainMenuScene::MainMenuScene() : m_engine(nullptr) {}
MainMenuScene::~MainMenuScene() {}

void MainMenuScene::Initialize(Engine *engine) {
  Scene2D::Initialize(engine);
  m_engine = engine;

  // Pre-load aircraft database from bundle or disk
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

  // Option 0: Quick Flight
  if ((selectTriggered && m_selectedOption == 0)) {
    m_engine->GetSceneManager().LoadScene("FlightScene");
  }

  // Option 1: Hangar / Plane Selector
  if ((selectTriggered && m_selectedOption == 1)) {
    m_engine->GetSceneManager().LoadScene("HangarScene");
  }

  // Option 2: Exit
  if ((selectTriggered && m_selectedOption == 2) || IsKeyPressed(KEY_ESCAPE)) {
    CloseWindow();
  }
}

void MainMenuScene::Draw() {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  // Background Gradient
  DrawRectangleGradientV(0, 0, screenW, screenH, Color{15, 20, 30, 255},
                         Color{5, 8, 12, 255});

  // Decorative Horizon Lines / Grid
  for (int y = screenH / 2; y < screenH; y += 40) {
    DrawLine(0, y, screenW, y, Fade(LIME, 0.08f));
  }

  // Title
  const char *title = "BUDGET FLIGHT SIMULATOR";
  // const char *sub = "TACTICAL AERODYNAMICS & FLIGHT DYNAMICS";
  DrawText(title, screenW / 2 - MeasureText(title, 36) / 2, screenH / 5, 36,
           LIME);
  // DrawText(sub, screenW / 2 - MeasureText(sub, 16) / 2, screenH / 5 + 45, 16,
  // LIGHTGRAY);

  // Menu Buttons
  float btnWidth = 320.0f;
  float btnHeight = 50.0f;
  float startY = screenH * 0.45f;
  float gap = 20.0f;

  Rectangle btnFly = {(screenW - btnWidth) / 2.0f, startY, btnWidth, btnHeight};
  Rectangle btnHangar = {(screenW - btnWidth) / 2.0f,
                         startY + (btnHeight + gap), btnWidth, btnHeight};
  Rectangle btnExit = {(screenW - btnWidth) / 2.0f,
                       startY + (btnHeight + gap) * 2, btnWidth, btnHeight};

  if (DrawButton("FLY TEST SCENE", btnFly, m_selectedOption == 0)) {
    m_engine->GetSceneManager().LoadScene("FlightScene");
  }
  if (DrawButton("SELECT AIRCRAFT", btnHangar, m_selectedOption == 1)) {
    m_engine->GetSceneManager().LoadScene("HangarScene");
  }
  if (DrawButton("EXIT TO DESKTOP", btnExit, m_selectedOption == 2)) {
    CloseWindow();
  }

  // Footer Info
  DrawText("Use [UP / DOWN] and [ENTER] or Mouse to navigate", 20, screenH - 30,
           14, DARKGRAY);
}

void MainMenuScene::Shutdown() {}
