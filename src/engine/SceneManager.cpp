#include "engine/SceneManager.h"
#include "engine/Scene.h"
#include "engine/Engine.h"

SceneManager::SceneManager() : m_engine(nullptr), m_activeScene(nullptr) {}

SceneManager::~SceneManager() {}

void SceneManager::Initialize(Engine* engine) {
    m_engine = engine;
}

void SceneManager::OnResize(int width, int height) {
    if (m_activeScene) {
        m_activeScene->OnResize(width, height);
    }
}

void SceneManager::LoadScene(const std::string& name) {
    if (m_activeScene) {
        m_activeScene->Shutdown();
    }

    auto it = m_scenes.find(name);
    if (it != m_scenes.end()) {
        m_activeScene = it->second.get();
        m_activeScene->Initialize(m_engine); // Pass the engine pointer
    }
}

void SceneManager::Update(float deltaTime) {
    if (m_activeScene) {
        m_activeScene->Update(deltaTime);
    }
}

void SceneManager::Draw() {
    if (m_activeScene) {
        m_activeScene->Draw();
    }
}

void SceneManager::Shutdown() {
    if (m_activeScene) {
        m_activeScene->Shutdown();
    }
}
