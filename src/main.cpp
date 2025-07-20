#include "asset_loader.hpp"
#include "car.h"
#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"

// asset loader

#include <filesystem> // For temporary file creation
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Helper function to load model from asset data (via temporary file)
Model LoadModelFromAssetData(const std::vector<char> &asset_data,
                             const std::string &original_filename) {
  if (asset_data.empty()) {
    return {0}; // Return empty model
  }

  // Create a temporary file
  std::string temp_filename = fs::temp_directory_path().string() + "/" +
                              fs::path(original_filename).filename().string();
  std::ofstream temp_ofs(temp_filename, std::ios::binary);
  if (!temp_ofs.is_open()) {
    std::cerr << "Error: Could not create temporary file for asset: "
              << original_filename << std::endl;
    return {0};
  }
  temp_ofs.write(reinterpret_cast<const char *>(asset_data.data()),
                 asset_data.size());
  temp_ofs.close();

  // Load model from temporary file
  Model model = LoadModel(temp_filename.c_str());

  // Delete temporary file
  fs::remove(temp_filename);

  return model;
}

int main() {
  int screenWidth = 1280;
  int screenHeight = 720;

  AssetPacking::AssetLoader asset_loader;
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
  InitWindow(screenWidth, screenHeight, "the sheep game");
  SetTargetFPS(60);
  // ToggleFullscreen();

  rlImGuiSetup(true); // true: use dark theme, false: use light theme

  bool my_tool_active = true;

  Camera camera;

  float fov = 45.0f;

  camera.position = Vector3{5.0, 20.0, 5.0};
  camera.up = Vector3{0.0f, 1.0f, 0.0f};
  camera.projection = CAMERA_PERSPECTIVE; // Camera mode type

  Car car; // Create a car instance

  camera.target = car.GetPosition();

  asset_loader.loadPackedFile("asset.pack");

  // Load the car model from the asset loader
  Model car_model = LoadModelFromAssetData(
      asset_loader.getAssetData("sheep.glb"), "sheep.glb");

  while (!WindowShouldClose()) {

    camera.fovy = fov;                          // Camera field-of-view Y
    UpdateCamera(&camera, CAMERA_FIRST_PERSON); // Update camera
    // car.Update(GetFrameTime());                 // Update car position

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(camera);

    DrawModel(car_model, {5, 0, 5}, 10, WHITE);
    DrawModelEx(car_model, {5, 20, 0}, {1, 2, 3}, 180.0f, {10.0f, 10.0f, 10.0f},
                WHITE);
    // car.Draw(); // Draw the car

    DrawGrid(10, 10);

    EndMode3D();

    // ===IMGUI===
    rlImGuiBegin(); // Start ImGui frame
    if (my_tool_active) {
      EnableCursor(); // Enable cursor if ImGui tool is active
    } else {
      DisableCursor(); // Disable cursor for 3D interaction
    }

    if (my_tool_active) {
      ImGui::Begin("Game Settings");

      ImGui::SliderFloat("FOV", &fov, 0, 300);

      ImGui::Text("Camera FOV: %.2f", fov);
      ImGui::Text("Camera Position: %.2f, %.2f, %.2f", camera.position.x,
                  camera.position.y, camera.position.z);

      ImGui::Text("Camera Target: %.2f, %.2f, %.2f", camera.target.x,
                  camera.target.y, camera.target.z);

      // Car controls
      ImGui::Text("Car Position: %.2f, %.2f, %.2f", car.GetPosition().x,
                  car.GetPosition().y, car.GetPosition().z);
      ImGui::Text("Car Scale: %.2f", car.scale);
      ImGui::SliderFloat("Car Scale", &car.scale, 0.1f, 5.0f);

      if (ImGui::Button("CloseWindow")) {
        my_tool_active = false;
      }

      ImGui::End();
    }
    rlImGuiEnd(); // Render ImGui draw data
    // ===================================

    EndDrawing();

    if (IsKeyPressed(KEY_M)) {
      my_tool_active = !my_tool_active;
    }
  }

  EnableCursor();

  rlImGuiShutdown(); // Shutdown ImGui
  CloseWindow();
  return 0;
}
