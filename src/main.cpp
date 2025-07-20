#include "car.h"
#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"

int main() {
  int screenWidth = 1280;
  int screenHeight = 720;

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
  while (!WindowShouldClose()) {

    camera.fovy = fov;                          // Camera field-of-view Y
    UpdateCamera(&camera, CAMERA_FIRST_PERSON); // Update camera
    // car.Update(GetFrameTime());                 // Update car position

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(camera);

    car.Draw(); // Draw the car

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
