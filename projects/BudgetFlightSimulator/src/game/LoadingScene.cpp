#include "game/LoadingScene.h"
#include "engine/Engine.h"
#include "engine/SceneManager.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>

LoadingScene::LoadingScene() : m_engine(nullptr) {}
LoadingScene::~LoadingScene() { Shutdown(); }

void LoadingScene::Initialize(Engine *engine) {
  Scene2D::Initialize(engine);
  m_engine = engine;
  m_framesWaited = 0;
  m_spinnerAngle = 0.0f;

  const char *fontPath = "DATA/FONTS/Audiowide-Regular.ttf";
  if (FileExists(fontPath)) {
    m_customFont = LoadFontEx(fontPath, 48, 0, 0);
    SetTextureFilter(m_customFont.texture, TEXTURE_FILTER_BILINEAR);
    m_fontLoaded = true;
  }

  std::vector<std::string> tips = {
      "TIP: Keep your airspeed above 130 km/h to avoid stalling.",
      "TIP: The Sea Duck can land on water. Retract gear before splashdown.",
      "TIP: Press [C] to lock on and fire homing missiles at enemies.",
      "TIP: Use [F1] to toggle cinematic camera modes.",
      "TIP: Heavy payloads reduce maneuverability. Drop them with [G]."};
  m_randomTip = tips[GetRandomValue(0, tips.size() - 1)];

  DisableCursor();
}

void LoadingScene::Update(float deltaTime) {
  m_spinnerAngle += deltaTime * 400.0f;

  m_framesWaited++;
  if (m_framesWaited == 3) {
    m_engine->GetSceneManager().LoadScene("FlightScene");
  }
}

void LoadingScene::Draw() {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  DrawRectangleGradientV(0, 0, screenW, screenH, Color{10, 16, 26, 255},
                         Color{2, 4, 8, 255});

  for (int y = 0; y < screenH; y += 40)
    DrawLine(0, y, screenW, y, Fade(Color{20, 40, 60, 255}, 0.3f));
  for (int x = 0; x < screenW; x += 40)
    DrawLine(x, 0, x, screenH, Fade(Color{20, 40, 60, 255}, 0.3f));

  const char *title = "INITIALIZING AVIONICS...";
  int fontSize = 32;
  Vector2 textPos = {(float)screenW / 2.0f -
                         MeasureText(title, fontSize) / 2.0f,
                     screenH * 0.45f};

  if (m_fontLoaded) {
    textPos.x = screenW / 2.0f -
                MeasureTextEx(m_customFont, title, fontSize, 1.0f).x / 2.0f;
    DrawTextEx(m_customFont, title, Vector2{textPos.x + 2.0f, textPos.y + 2.0f},
               fontSize, 1.0f, Fade(BLACK, 0.6f));
    DrawTextEx(m_customFont, title, textPos, fontSize, 1.0f,
               Color{100, 240, 255, 255});
  } else {
    DrawText(title, (int)textPos.x, (int)textPos.y, fontSize,
             Color{100, 240, 255, 255});
  }

  Vector2 center = {(float)screenW / 2.0f, screenH * 0.35f};
  float radius = 40.0f;
  DrawCircleLines((int)center.x, (int)center.y, radius,
                  Fade(Color{100, 240, 255, 255}, 0.5f));
  DrawLineEx(center,
             Vector2{center.x + cosf(m_spinnerAngle * DEG2RAD) * radius,
                     center.y + sinf(m_spinnerAngle * DEG2RAD) * radius},
             3.0f, Color{255, 215, 0, 255});

  int tipFontSize = 18;
  Vector2 tipPos = {(float)screenW / 2.0f -
                        MeasureText(m_randomTip.c_str(), tipFontSize) / 2.0f,
                    screenH * 0.85f};
  if (m_fontLoaded) {
    tipPos.x =
        screenW / 2.0f -
        MeasureTextEx(m_customFont, m_randomTip.c_str(), tipFontSize, 1.0f).x /
            2.0f;
    DrawTextEx(m_customFont, m_randomTip.c_str(), tipPos, tipFontSize, 1.0f,
               LIGHTGRAY);
  } else {
    DrawText(m_randomTip.c_str(), (int)tipPos.x, (int)tipPos.y, tipFontSize,
             LIGHTGRAY);
  }
}

void LoadingScene::Shutdown() {
  if (m_fontLoaded) {
    UnloadFont(m_customFont);
    m_fontLoaded = false;
  }
}
