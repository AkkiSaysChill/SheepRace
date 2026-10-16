#include "game/ComicScene.h"
#include "engine/Engine.h"
#include "engine/SceneManager.h"
#include "game/FlightSession.h"

ComicScene::ComicScene() : m_engine(nullptr) {}
ComicScene::~ComicScene() { Shutdown(); }

void ComicScene::Initialize(Engine *engine) {
  Scene2D::Initialize(engine);
  m_engine = engine;

  m_currentPanelIndex = 0;
  m_panelTimer = 0.0f;
  m_fadeAlpha = 1.0f;
  m_isFadingOut = false;

  if (FileExists("DATA/FONTS/Audiowide-Regular.ttf")) {
    m_comicFont = LoadFontEx("DATA/FONTS/Audiowide-Regular.ttf", 36, 0, 0);
    SetTextureFilter(m_comicFont.texture, TEXTURE_FILTER_BILINEAR);
  }

  if (FlightSession::GetInstance().activeComicSequence.empty()) {
    m_engine->GetSceneManager().LoadScene(
        FlightSession::GetInstance().sceneAfterComic);
    return;
  }

  m_engine->GetAssetLoader().loadPackedFile("DATA/COMIC/comic.bin");
  m_engine->GetAssetLoader().loadPackedFile("DATA/VOICEOVERS/voiceovers.bin");

  LoadCurrentPanel();
}

void ComicScene::LoadCurrentPanel() {
  auto &seq = FlightSession::GetInstance().activeComicSequence;
  if (m_currentPanelIndex >= seq.size())
    return;

  const auto &panel = seq[m_currentPanelIndex];
  auto &assetLoader = m_engine->GetAssetLoader();

  TraceLog(LOG_INFO, "--------------------------------------------------");
  TraceLog(LOG_INFO, "[COMIC] Loading Panel %d / %d", m_currentPanelIndex + 1,
           (int)seq.size());

  // --- 1. Load Image ---
  auto imgData = assetLoader.getAssetData(panel.imagePath);
  if (!imgData.empty()) {
    Image img = LoadImageFromMemory(
        ".png", (const unsigned char *)imgData.data(), (int)imgData.size());
    m_currentImage = LoadTextureFromImage(img);
    UnloadImage(img);
  } else {
    TraceLog(LOG_WARNING, "[COMIC] Image NOT FOUND: %s",
             panel.imagePath.c_str());
  }

  // --- 2. Load Voiceover as SOUND ---
  auto audData = assetLoader.getAssetData(panel.audioPath);

  if (!audData.empty()) {
    TraceLog(LOG_INFO, "[COMIC] Audio found in packed file. Size: %d bytes.",
             (int)audData.size());

    std::string ext = ".wav";
    size_t dotPos = panel.audioPath.find_last_of('.');
    if (dotPos != std::string::npos) {
      ext = panel.audioPath.substr(dotPos);
    }

    // Safely load short audio clips via the Wave system
    Wave wave =
        LoadWaveFromMemory(ext.c_str(), (const unsigned char *)audData.data(),
                           (int)audData.size());
    m_currentVoiceover = LoadSoundFromWave(wave);
    UnloadWave(wave);
  } else {
    TraceLog(LOG_WARNING, "[COMIC] Audio NOT FOUND in packed file. Falling "
                          "back to local disk...");

    if (FileExists(panel.audioPath.c_str())) {
      m_currentVoiceover = LoadSound(panel.audioPath.c_str());
    } else {
      std::string altPath = "DATA/VOICEOVERS/" + panel.audioPath;
      if (FileExists(altPath.c_str())) {
        m_currentVoiceover = LoadSound(altPath.c_str());
      } else {
        TraceLog(LOG_ERROR, "[COMIC] Audio file completely missing!");
      }
    }
  }

  // Play Sound if loaded correctly (Check frameCount to ensure valid sound
  // data)
  if (m_currentVoiceover.frameCount > 0) {
    TraceLog(LOG_INFO, "[COMIC] SUCCESS: Sound loaded and playing.");
    SetSoundVolume(m_currentVoiceover, 1.0f);
    PlaySound(m_currentVoiceover);
  } else {
    TraceLog(LOG_ERROR, "[COMIC] FAILED: Sound frameCount is 0.");
  }
  TraceLog(LOG_INFO, "--------------------------------------------------");

  m_panelTimer = 0.0f;
  m_fadeAlpha = 1.0f;
  m_isFadingOut = false;
}

void ComicScene::UnloadCurrentPanel() {
  if (m_currentImage.id != 0) {
    UnloadTexture(m_currentImage);
    m_currentImage = {0};
  }
  if (m_currentVoiceover.frameCount > 0) {
    StopSound(m_currentVoiceover);
    UnloadSound(m_currentVoiceover);
    m_currentVoiceover = {0};
  }
}

void ComicScene::Update(float deltaTime) {
  auto &seq = FlightSession::GetInstance().activeComicSequence;
  if (seq.empty())
    return;

  m_panelTimer += deltaTime;

  // Fade in
  if (!m_isFadingOut && m_fadeAlpha > 0.0f) {
    m_fadeAlpha -= deltaTime * 1.5f;
    if (m_fadeAlpha < 0.0f)
      m_fadeAlpha = 0.0f;
  }

  // Fade out and transition
  if (m_isFadingOut) {
    m_fadeAlpha += deltaTime * 2.0f;
    if (m_fadeAlpha >= 1.0f) {
      UnloadCurrentPanel();
      m_currentPanelIndex++;

      if (m_currentPanelIndex >= seq.size()) {
        m_engine->GetSceneManager().LoadScene(
            FlightSession::GetInstance().sceneAfterComic);
      } else {
        LoadCurrentPanel();
      }
    }
    return;
  }

  // Auto-advance logic
  bool autoAdvance = false;
  if (m_currentVoiceover.frameCount > 0) {
    // Wait until the sound finishes playing (plus a tiny 0.5s buffer for
    // safety)
    if (!IsSoundPlaying(m_currentVoiceover) && m_panelTimer > 0.5f) {
      autoAdvance = true;
    }
  } else if (m_panelTimer > 4.0f) {
    // Fallback: wait 4 seconds if no audio exists
    autoAdvance = true;
  }

  if (autoAdvance || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
    m_isFadingOut = true;
  }
}

void ComicScene::Draw() {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  DrawRectangle(0, 0, screenW, screenH, BLACK);

  auto &seq = FlightSession::GetInstance().activeComicSequence;
  if (m_currentPanelIndex >= seq.size())
    return;
  const auto &panel = seq[m_currentPanelIndex];

  // Draw Image
  if (m_currentImage.id != 0) {
    float scale = fminf((float)screenW / m_currentImage.width,
                        (float)screenH / m_currentImage.height);
    Rectangle source = {0.0f, 0.0f, (float)m_currentImage.width,
                        (float)m_currentImage.height};
    Rectangle dest = {(screenW - (m_currentImage.width * scale)) / 2.0f,
                      (screenH - (m_currentImage.height * scale)) / 2.0f,
                      m_currentImage.width * scale,
                      m_currentImage.height * scale};
    DrawTexturePro(m_currentImage, source, dest, Vector2{0, 0}, 0.0f, WHITE);
  }

  // Draw Subtitles
  if (!panel.subtitle.empty()) {
    int fontSize = 24;
    float textW =
        m_comicFont.texture.id != 0
            ? MeasureTextEx(m_comicFont, panel.subtitle.c_str(), fontSize, 1.0f)
                  .x
            : MeasureText(panel.subtitle.c_str(), fontSize);

    Rectangle subBg = {0, (float)screenH - 100, (float)screenW, 100};
    DrawRectangleGradientV((int)subBg.x, (int)subBg.y, (int)subBg.width,
                           (int)subBg.height, Fade(BLACK, 0.0f),
                           Fade(BLACK, 0.8f));

    Vector2 textPos = {(screenW - textW) / 2.0f, screenH - 60.0f};
    if (m_comicFont.texture.id != 0) {
      DrawTextEx(m_comicFont, panel.subtitle.c_str(),
                 Vector2{textPos.x + 2, textPos.y + 2}, fontSize, 1.0f, BLACK);
      DrawTextEx(m_comicFont, panel.subtitle.c_str(), textPos, fontSize, 1.0f,
                 RAYWHITE);
    } else {
      DrawText(panel.subtitle.c_str(), textPos.x, textPos.y, fontSize,
               RAYWHITE);
    }
  }

  DrawText("PRESS [SPACE] TO SKIP", screenW - 220, 20, 16,
           Fade(LIGHTGRAY, 0.5f));

  if (m_fadeAlpha > 0.0f) {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, m_fadeAlpha));
  }
}

void ComicScene::Shutdown() {
  UnloadCurrentPanel();
  if (m_comicFont.texture.id != 0) {
    UnloadFont(m_comicFont);
    m_comicFont = {0};
  }
}
