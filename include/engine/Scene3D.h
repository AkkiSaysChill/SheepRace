#pragma once

#include "engine/Scene.h"

class Scene3D : public Scene {
public:
    void Initialize(Engine* engine) override;

    // Default implementations for scenes that don't need them
    void Update(float deltaTime) override {}
    void Draw() override {}
    void Shutdown() override {}
};
