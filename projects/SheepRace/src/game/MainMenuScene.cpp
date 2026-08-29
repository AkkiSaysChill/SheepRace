#include "game/MainMenuScene.h"
#include "engine/Engine.h"
#include "engine/SceneManager.h"
#include "imgui.h"
#include "iostream"
#include "raylib.h"
#include "rlImGui.h"

MainMenuScene::MainMenuScene() : m_sceneManager(nullptr) {}

MainMenuScene::~MainMenuScene() {}

void MainMenuScene::Initialize(Engine* engine) {
  Scene2D::Initialize(engine); // Set up the 2D camera
  m_sceneManager = &engine->GetSceneManager();
}

void MainMenuScene::Update(float deltaTime) {
  // No update logic needed for the main menu
}

void MainMenuScene::Draw() {
  ClearBackground(RAYWHITE);
  DrawText("Sheep Race", 250, 200, 80, BLACK);

  rlImGuiBegin();
  ImGui::SetNextWindowPos(
      ImVec2(GetScreenWidth() / 2 - 100, GetScreenHeight() / 2));
  ImGui::SetNextWindowSize(ImVec2(200, 100));
  ImGui::Begin("Main Menu", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

  if (ImGui::Button("Start Game", ImVec2(180, 40))) {
    m_sceneManager->LoadScene("GameScene");
  }

  if (ImGui::Button("Exit", ImVec2(180, 40))) {
    std::cout << "aah" << std::endl;
    m_sceneManager->Shutdown();
    rlImGuiShutdown();
    CloseWindow();
  }

  ImGui::End();
  rlImGuiEnd();
}

void MainMenuScene::Shutdown() {
  // No shutdown logic needed for the main menu
}
