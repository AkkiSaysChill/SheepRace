#include "engine/LevelEditor.h"
#include "engine/PhysicsComponent.h" // Physics ko sync karne ke liye
#include "raymath.h"
#include <fstream>
#include <iostream>

void LevelEditor::ToggleEditor() {
  m_isActive = !m_isActive;
  if (!m_isActive)
    m_selectedObject = nullptr;
}

void LevelEditor::Update(
    Camera3D &camera, std::vector<std::shared_ptr<GameObject>> &gameObjects) {
  if (!m_isActive)
    return;

  float deltaTime = GetFrameTime();

  // ========================================
  // 1. CUSTOM EDITOR CAMERA (Independent)
  // ========================================

  // A. Mouse sirf tabhi disable hoga jab button PRESS kiya ho
  if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
    DisableCursor();
  }

  // B. Ghumane ka logic
  if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
    Vector2 mouseDelta = GetMouseDelta();
    m_cameraYaw -= mouseDelta.x * 0.003f;
    m_cameraPitch -= mouseDelta.y * 0.003f;

    if (m_cameraPitch > 1.5f)
      m_cameraPitch = 1.5f;
    if (m_cameraPitch < -1.5f)
      m_cameraPitch = -1.5f;
  }

  // C. Jaise hi button chhoda, mouse wapas aa jayega!
  if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) {
    EnableCursor();
  }

  // Camera Direction Vectors
  Vector3 forward = {cosf(m_cameraPitch) * sinf(m_cameraYaw),
                     sinf(m_cameraPitch),
                     cosf(m_cameraPitch) * cosf(m_cameraYaw)};
  Vector3 rightDir = {-forward.z, 0.0f, forward.x};
  Vector3 upDir = {0.0f, 1.0f, 0.0f};

  // Fly logic (W A S D)
  float flySpeed =
      IsKeyDown(KEY_LEFT_SHIFT) ? 30.0f * deltaTime : 10.0f * deltaTime;

  if (IsKeyDown(KEY_W))
    camera.position =
        Vector3Add(camera.position, Vector3Scale(forward, flySpeed));
  if (IsKeyDown(KEY_S))
    camera.position =
        Vector3Subtract(camera.position, Vector3Scale(forward, flySpeed));
  if (IsKeyDown(KEY_D))
    camera.position =
        Vector3Add(camera.position, Vector3Scale(rightDir, flySpeed));
  if (IsKeyDown(KEY_A))
    camera.position =
        Vector3Subtract(camera.position, Vector3Scale(rightDir, flySpeed));
  if (IsKeyDown(KEY_SPACE))
    camera.position =
        Vector3Add(camera.position, Vector3Scale(upDir, flySpeed));
  if (IsKeyDown(KEY_LEFT_CONTROL))
    camera.position =
        Vector3Subtract(camera.position, Vector3Scale(upDir, flySpeed));

  camera.target = Vector3Add(camera.position, forward);

  // ========================================
  // 2. EDITOR TOOLS (Transform & Raycast)
  // ========================================

  // Mode Switching
  if (IsKeyPressed(KEY_T))
    m_currentMode = TransformMode::TRANSLATE;
  if (IsKeyPressed(KEY_R))
    m_currentMode = TransformMode::ROTATE;
  if (IsKeyPressed(KEY_E))
    m_currentMode = TransformMode::SCALE;

  // Object Selection (Raycast)
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    float closestHit = 99999.0f;
    m_selectedObject = nullptr;

    for (auto &obj : gameObjects) {
      BoundingBox box = obj->GetBoundingBox();
      RayCollision collision = GetRayCollisionBox(ray, box);
      if (collision.hit && collision.distance < closestHit) {
        closestHit = collision.distance;
        m_selectedObject = obj;
      }
    }
  }
  // --- MODE SWITCHING (T = Translate, R = Rotate, E = Scale) ---
  if (IsKeyPressed(KEY_T))
    m_currentMode = TransformMode::TRANSLATE;
  if (IsKeyPressed(KEY_R))
    m_currentMode = TransformMode::ROTATE;
  if (IsKeyPressed(KEY_E))
    m_currentMode = TransformMode::SCALE;

  // --- MOUSE SELECTION ---
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    float closestHit = 99999.0f;
    m_selectedObject = nullptr;

    for (auto &obj : gameObjects) {
      BoundingBox box = obj->GetBoundingBox();
      RayCollision collision = GetRayCollisionBox(ray, box);
      if (collision.hit && collision.distance < closestHit) {
        closestHit = collision.distance;
        m_selectedObject = obj;
      }
    }
  }

  // --- TRANSFORM LOGIC (SCROLL WHEEL + X, Y, Z) ---
  if (m_selectedObject != nullptr) {
    float wheel = GetMouseWheelMove(); // Gives 1.0 or -1.0 when scrolling

    if (wheel != 0.0f) {
      float modifier = IsKeyDown(KEY_LEFT_SHIFT) ? 2.0f : 1.0f;
      bool moved = false;

      // 1. TRANSLATE
      if (m_currentMode == TransformMode::TRANSLATE) {
        float delta = wheel * m_moveSpeed * modifier;
        // Agar X, Y, Z me se koi nahi dabaya, toh X par move karo
        if (!IsKeyDown(KEY_X) && !IsKeyDown(KEY_Y) && !IsKeyDown(KEY_Z)) {
          m_selectedObject->position.x += delta;
          moved = true;
        } else {
          if (IsKeyDown(KEY_X)) {
            m_selectedObject->position.x += delta;
            moved = true;
          }
          if (IsKeyDown(KEY_Y)) {
            m_selectedObject->position.y += delta;
            moved = true;
          }
          if (IsKeyDown(KEY_Z)) {
            m_selectedObject->position.z += delta;
            moved = true;
          }
        }
      }

      // 2. ROTATE
      else if (m_currentMode == TransformMode::ROTATE) {
        float delta = wheel * m_rotateSpeed * modifier;
        Vector3 euler = QuaternionToEuler(m_selectedObject->rotation);

        if (!IsKeyDown(KEY_X) && !IsKeyDown(KEY_Y) && !IsKeyDown(KEY_Z)) {
          euler.y += delta;
          moved = true; // Default Y (Ghumana)
        } else {
          if (IsKeyDown(KEY_X)) {
            euler.x += delta;
            moved = true;
          }
          if (IsKeyDown(KEY_Y)) {
            euler.y += delta;
            moved = true;
          }
          if (IsKeyDown(KEY_Z)) {
            euler.z += delta;
            moved = true;
          }
        }

        if (moved)
          m_selectedObject->rotation =
              QuaternionFromEuler(euler.x, euler.y, euler.z);
      }

      // 3. SCALE
      else if (m_currentMode == TransformMode::SCALE) {
        float delta = wheel * m_scaleSpeed * modifier;
        // If no axis key is held, scale uniformly!
        if (!IsKeyDown(KEY_X) && !IsKeyDown(KEY_Y) && !IsKeyDown(KEY_Z)) {
          m_selectedObject->scale.x += delta;
          m_selectedObject->scale.y += delta;
          m_selectedObject->scale.z += delta;
          moved = true;
        } else {
          if (IsKeyDown(KEY_X)) {
            m_selectedObject->scale.x += delta;
            moved = true;
          }
          if (IsKeyDown(KEY_Y)) {
            m_selectedObject->scale.y += delta;
            moved = true;
          }
          if (IsKeyDown(KEY_Z)) {
            m_selectedObject->scale.z += delta;
            moved = true;
          }
        }
      }

      // --- SYNC WITH BULLET PHYSICS ---
      if (moved) {
        auto phys = m_selectedObject->GetComponent<PhysicsComponent>();
        if (phys && phys->GetRigidBody()) {
          btTransform trans;
          phys->GetRigidBody()->getMotionState()->getWorldTransform(trans);

          // Update Position
          trans.setOrigin(btVector3(m_selectedObject->position.x,
                                    m_selectedObject->position.y,
                                    m_selectedObject->position.z));

          // Update Rotation
          trans.setRotation(btQuaternion(
              m_selectedObject->rotation.x, m_selectedObject->rotation.y,
              m_selectedObject->rotation.z, m_selectedObject->rotation.w));

          phys->GetRigidBody()->setWorldTransform(trans);
          phys->GetRigidBody()->getMotionState()->setWorldTransform(trans);

          // Update Scale (Bullet requires setting local scale on the collision
          // shape)
          phys->GetRigidBody()->getCollisionShape()->setLocalScaling(
              btVector3(m_selectedObject->scale.x, m_selectedObject->scale.y,
                        m_selectedObject->scale.z));
        }
      }
    }
  }
}

void LevelEditor::Draw() {
  if (m_isActive) {
    // UI TEXT FOR EDITOR
    DrawText("EDITOR MODE ACTIVE (Press TAB to exit)", 10, 10, 20, GREEN);

    std::string modeStr = "Mode: ";
    if (m_currentMode == TransformMode::TRANSLATE)
      modeStr += "TRANSLATE (T)";
    else if (m_currentMode == TransformMode::ROTATE)
      modeStr += "ROTATE (R)";
    else if (m_currentMode == TransformMode::SCALE)
      modeStr += "SCALE (E)";
    DrawText(modeStr.c_str(), 10, 40, 20, YELLOW);

    DrawText("Hold X, Y, or Z and Scroll Mouse Wheel!", 10, 70, 20, WHITE);

    if (m_selectedObject != nullptr) {
      DrawText(("Selected: " + m_selectedObject->name).c_str(), 10, 100, 20,
               ORANGE);

      // Draw Bounding Box around selected object
      DrawBoundingBox(m_selectedObject->GetBoundingBox(), RED);

      // Draw a mini XYZ Axis indicator at the object's position (Visual Helper)
      Vector3 pos = m_selectedObject->position;
      DrawLine3D(pos, {pos.x + 3.0f, pos.y, pos.z}, RED);   // X Axis = RED
      DrawLine3D(pos, {pos.x, pos.y + 3.0f, pos.z}, GREEN); // Y Axis = GREEN
      DrawLine3D(pos, {pos.x, pos.y, pos.z + 3.0f}, BLUE);  // Z Axis = BLUE
    }
  }
}

std::vector<ObjectSaveData>
LevelEditor::LoadLevelData(const std::string &filename) {
  std::vector<ObjectSaveData> levelData;
  std::ifstream inFile(filename, std::ios::binary);

  // Agar file nahi mili (jaise first time run pe), toh khali data return karo
  if (!inFile.is_open()) {
    std::cout << "[EDITOR] No save file found at " << filename
              << ". Loading default.\n";
    return levelData;
  }

  // File se data read karna
  size_t count = 0;
  inFile.read(reinterpret_cast<char *>(&count), sizeof(size_t));

  levelData.resize(count);
  inFile.read(reinterpret_cast<char *>(levelData.data()),
              count * sizeof(ObjectSaveData));
  inFile.close();

  std::cout << "[EDITOR] SUCCESS: Loaded " << count << " objects from "
            << filename << "!\n";
  return levelData;
}

// 3. SAVER
void LevelEditor::SaveLevel(
    const std::string &filename,
    const std::vector<std::shared_ptr<GameObject>> &gameObjects) {
  std::vector<ObjectSaveData> levelData;

  for (auto &obj : gameObjects) {
    ObjectSaveData data;
    data.objectType = obj->typeID;
    data.position = obj->position;
    // YEH LINE THEEK KARNI HAI (Quaternion ko Vector3 me save)
    data.rotation = QuaternionToEuler(obj->rotation);
    data.scale = obj->scale;
    levelData.push_back(data);
  }

  std::ofstream outFile(filename, std::ios::binary);
  if (outFile.is_open()) {
    size_t count = levelData.size();
    outFile.write(reinterpret_cast<char *>(&count), sizeof(size_t));
    outFile.write(reinterpret_cast<char *>(levelData.data()),
                  count * sizeof(ObjectSaveData));
    outFile.close();
    std::cout << "SUCCESS: Level Saved to " << filename << "!\n";
  }
}
