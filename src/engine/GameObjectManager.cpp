#include "engine/GameObjectManager.h"
#include "engine/DrawableComponent.h"
#include <algorithm>
#include <iostream>

GameObjectManager::GameObjectManager() {}

GameObjectManager::~GameObjectManager() {}

void GameObjectManager::RemoveGameObject(std::shared_ptr<GameObject> gameObject) {
    auto& list = m_GameObjects;
    list.erase(std::remove(list.begin(), list.end(), gameObject), list.end());
}

void GameObjectManager::AddGameObject(std::shared_ptr<GameObject> gameObject) {
    
    if (!gameObject) {
        std::cerr << "[Error] Attempted to add NULL GameObject to manager!" << std::endl;
        return;
    }
    m_GameObjects.push_back(gameObject);
}

void GameObjectManager::Update(float dt) {
    for (auto& gameObject : m_GameObjects) {
        gameObject->Update(dt);
    }
}

void GameObjectManager::Draw() {
    for (auto& gameObject : m_GameObjects) {
        if (auto drawable = gameObject->GetComponent<DrawableComponent>()) {
            drawable->Draw();
        }
    }
}