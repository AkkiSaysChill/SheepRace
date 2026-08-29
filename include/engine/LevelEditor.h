#pragma once

#include "engine/GameObject.h"
#include "engine/GameObjectManager.h"
#include "engine/levelData.h"
#include "raylib.h"
#include <memory>
#include <string>
#include <vector>

enum class TransformMode { TRANSLATE, ROTATE, SCALE };

class LevelEditor {
public:
  LevelEditor() = default;
  ~LevelEditor() = default;

  void ToggleEditor();
  bool IsActive() const { return m_isActive; }

  void Update(Camera3D &camera,
              std::vector<std::shared_ptr<GameObject>> &gameObjects);
  void Draw(); // For UI and Gizmos

  void SyncCamera(float yaw, float pitch) {
    m_cameraYaw = yaw;
    m_cameraPitch = pitch;
  }

  void SaveLevel(const std::string &filename,
                 const std::vector<std::shared_ptr<GameObject>> &gameObjects);
  std::vector<ObjectSaveData> LoadLevelData(const std::string &filename);

private:
  bool m_isActive = false;
  std::shared_ptr<GameObject> m_selectedObject = nullptr;

  TransformMode m_currentMode = TransformMode::TRANSLATE;

  // Base speeds for the mouse wheel
  float m_moveSpeed = 2.0f;
  float m_rotateSpeed = 0.5f;
  float m_scaleSpeed = 0.2f;

  float m_cameraYaw = 0.0f;
  float m_cameraPitch = 0.0f;
};
