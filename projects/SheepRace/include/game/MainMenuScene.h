#pragma once

#include "engine/Scene2D.h" // Inherit from Scene2D

class SceneManager; // Forward declaration
class Engine; // Forward declaration

class MainMenuScene : public Scene2D {
public:
    MainMenuScene();
    ~MainMenuScene() override;

    void Initialize(Engine* engine) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;

private:
    SceneManager* m_sceneManager;
};
