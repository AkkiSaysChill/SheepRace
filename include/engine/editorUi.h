#pragma once
#include "GameObjectManager.h"
#include "PhysicsSystem.h"
#include "PlayerProgress.h"
#include "asset_loader.hpp"

class EditorUI {
public:
	static void RenderHierarchy(GameObjectManager& scene, PhysicsSystem& physicsSystem, Shader defaultShader, PlayerProgress& m_progress, AssetPacking::AssetLoader& assetLoader);
};