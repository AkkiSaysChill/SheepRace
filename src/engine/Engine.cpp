#include "engine/Engine.h"
#include "engine/Scene.h"
#include "raylib.h"
#include "rlImGui.h"
#include "rlgl.h"
#include <raymath.h>

Engine::Engine()
    : m_screenWidth(1280), m_screenHeight(720), m_renderer(nullptr),
      m_postProcessing(nullptr) {}

Engine::~Engine() {}

void Engine::Initialize() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
  InitWindow(m_screenWidth, m_screenHeight, "Sheep Race");
  SetTargetFPS(60);
  ChangeDirectory(GetApplicationDirectory());
  SetExitKey(KEY_NULL);

  rlImGuiSetup(true);

  m_assetLoader.loadPackedFile("shaders.bin");
  m_assetLoader.loadPackedFile("effects.dyum");
  m_textManager.Initialize();

  m_renderer = new Renderer(m_screenWidth, m_screenHeight, m_assetLoader);
  m_postProcessing = new PostProcessing(m_assetLoader);

  m_sceneManager.Initialize(this);
  m_weatherSystem.Initialize(1400);

  m_camera.up = {0.0f, 1.0f, 0.0f};
  m_camera.projection = CAMERA_PERSPECTIVE;
  m_camera.fovy = 45.0f;
  m_camera.position = {0.0f, 5.0f, -10.0f};
  m_camera.target = {0.0f, 0.0f, 0.0f};
}

void Engine::resize() {
  m_screenWidth = GetScreenWidth();
  m_screenHeight = GetScreenHeight();
  rlViewport(0, 0, m_screenWidth, m_screenHeight);
  if (m_renderer)
    m_renderer->OnResize(m_screenWidth, m_screenHeight);
  m_sceneManager.OnResize(m_screenWidth, m_screenHeight);
}

void Engine::Run() {
  while (!WindowShouldClose()) {

    if (IsWindowResized())
      resize();

    float dt = GetFrameTime();
    m_sceneManager.Update(dt);

    // If an Exit button was pressed during Update, break out BEFORE rendering!
    if (ShouldQuit()) {
      break;
    }

    // ── Render frame ───────────────────────────────────────────
    // NOTE: BeginDrawing() is now called INSIDE Renderer::Render(),
    // right before the final blit, so the flow is:
    //
    //   Renderer::Render()
    //     → shadow pass     (into m_shadowMap RenderTexture)
    //     → main 3D pass    (into m_target RenderTexture)
    //         scene.Draw()          ← world geometry
    //         extraDraw3D()         ← particles, debug, etc.
    //     → BeginDrawing()
    //     → blit m_target to screen (with or without post-fx)
    //
    //   activeScene->Draw()         ← HUD, UI, 2D overlays ONLY
    //   EndDrawing()
    //
    Scene *activeScene = m_sceneManager.GetActiveScene();
    if (activeScene) {
      auto &dayNightLighting = m_dayNightCycle.GetCurrentLighting();
      auto &weather = m_weatherSystem.GetActiveProfile();

      Vector3 lightDir = dayNightLighting.lightDir;

      float shininess = 16.0f;
      float specStrength = 0.15f * weather.sunDimFactor;

      // Dynamic blend of DayNight clouds and Weather preset cloud cover
      float cloudCover = std::clamp(weather.cloudCover, 0.05f, 0.98f);
      float cloudFalloff = weather.cloudFalloff;

      float lightOrthoSize = 650.0f;

      m_renderer->Render(m_gameObjectManager, m_camera, *m_postProcessing,
                         lightDir, PostProcessingActive, m_camera.fovy,
                         shininess, specStrength, cloudCover, cloudFalloff,
                         lightOrthoSize, [&]() { activeScene->Draw3D(); });

      activeScene->Draw();
    }

    EndDrawing();

    if (IsKeyPressed(KEY_F11)) {
      ToggleFullscreen();
      resize();
    }
  }
}

void Engine::Shutdown() {
  m_sceneManager.Shutdown();
  delete m_renderer;
  delete m_postProcessing;
  m_textManager.Shutdown();
  rlImGuiShutdown();
  CloseWindow();
}
