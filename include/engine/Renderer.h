#pragma once
#include "engine/GameObjectManager.h"
#include "engine/PostProcessing.h"
#include "engine/asset_loader.hpp"
#include "functional"
#include "raylib.h"

using Draw3DCallback = std::function<void()>;

class Renderer {
public:
  Renderer(int screenWidth, int screenHeight,
           AssetPacking::AssetLoader &assetLoader);
  ~Renderer();

  void Render(GameObjectManager &scene, Camera &camera,
              PostProcessing &postProcessing, Vector3 &lightDir,
              bool postProcessingActive, float fov, float shininess,
              float specularStrength, float cloudCover, float cloudFalloff,
              float lightCamOrthoSize,
              Draw3DCallback extraDraw3D = nullptr); // <-- ADD THIS

  Shader &GetShadowShader();
  void OnResize(int width, int height);
  RenderTexture2D GetTarget() const { return m_target; }
  Texture2D GetMainTargetTexture() const { return m_target.texture; }
  RenderTexture2D GetReflectionTarget() const { return m_reflectionTarget; }

  Vector3 GetSkySunVector() const { return m_currentSkySunVector; }

private:
  Shader m_shadowShader;
  Shader m_skyShader;
  RenderTexture2D m_shadowMap;
  RenderTexture2D m_target;
  RenderTexture2D m_reflectionTarget;
  Texture2D m_noiseTexture;

  Vector3 m_currentSkySunVector = { -0.35f, 0.85f, -0.40f };
};
