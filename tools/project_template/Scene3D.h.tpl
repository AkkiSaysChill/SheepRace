#pragma once

#include "engine/Scene3D.h"

class %%SCENE_NAME%% : public Scene3D {
public:
    %%SCENE_NAME%%();
    ~%%SCENE_NAME%%() override;

    void Initialize(Engine* engine) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;

private:
    Engine* m_engine;
};
