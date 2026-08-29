#pragma once

#include "engine/Component.h"
#include <algorithm>
#include <memory>
#include <raylib.h>
#include <string>
#include <vector>

class GameObject {
public:
  Vector3 position = {0.0f, 0.0f, 0.0f};
  Quaternion rotation = {0.0f, 0.0f, 0.0f, 1.0f};
  Vector3 scale = {1.0f, 1.0f, 1.0f};

  int typeID = 0;

  GameObject() = default;
  ~GameObject() = default;

  // NEW: Thoda bada (generous) bounding box taaki Editor mein door se bhi click
  // ho jaye!
  BoundingBox GetBoundingBox() const {
    return {{position.x - scale.x, position.y - scale.y, position.z - scale.z},
            {position.x + scale.x, position.y + scale.y, position.z + scale.z}};
  }

  virtual void Update(float deltaTime) {
    for (auto &component : m_components) {
      component->Update(deltaTime);
    }
  }

  bool isDestructible = false;
  int health = 0;
  float timeToLive = 0.0f;

  template <typename T, typename... TArgs>
  std::shared_ptr<T> AddComponent(TArgs &&...args) {
    static_assert(std::is_base_of<Component, T>::value,
                  "T must be a descendant of Component");

    auto newComponent = std::make_shared<T>(std::forward<TArgs>(args)...);
    newComponent->SetOwner(this);
    m_components.push_back(newComponent);
    return newComponent;
  }

  template <typename T> std::shared_ptr<T> GetComponent() {
    for (auto &component : m_components) {
      auto castedComponent = std::dynamic_pointer_cast<T>(component);
      if (castedComponent) {
        return castedComponent;
      }
    }
    return nullptr;
  }

  std::string name = "GameObject";
  bool debugPhysics = false;

private:
  std::vector<std::shared_ptr<Component>> m_components;
};
