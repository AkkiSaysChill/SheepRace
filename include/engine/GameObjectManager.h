#pragma once

#include "engine/GameObject.h"
#include <memory>
#include <vector>

class GameObjectManager {
public:
  GameObjectManager();
  ~GameObjectManager();

  // Adds a GameObject to the scene and returns a raw pointer to it
  // The scene takes ownership of the GameObject
  template <typename T, typename... Args> T *CreateGameObject(Args &&...args) {
    auto gameObject = std::make_unique<T>(std::forward<Args>(args)...);
    T *ptr = gameObject.get();
    m_GameObjects.push_back(std::move(gameObject));
    return ptr;
  }

  // Add an existing shared_ptr GameObject to the scene
  void AddGameObject(std::shared_ptr<GameObject> gameObject);

  void Update(float dt);
  void Draw();

  // Finds the first GameObject of a given type
  template <typename T> T *FindObjectOfType() {
    for (const auto &go : m_GameObjects) {
      T *obj = dynamic_cast<T *>(go.get());
      if (obj) {
        return obj;
      }
    }
    return nullptr;
  }

  const std::vector<std::shared_ptr<GameObject>> &GetGameObjects() const {
    return m_GameObjects;
  }

  void RemoveGameObject(std::shared_ptr<GameObject> gameObject);

private:
  std::vector<std::shared_ptr<GameObject>> m_GameObjects;
};
