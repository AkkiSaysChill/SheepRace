#include "engine/PostProcessing.h"
#include "engine/asset_loader.hpp"
#include "raylib.h"
#include "rlgl.h"
#include <cstring>
#include <iostream>

PostProcessing::PostProcessing(AssetPacking::AssetLoader &assetLoader) {
  currentShader = FX_FXAA;

  const char *shader_names[] = {"glsl330/grayscale.fs",
                                "glsl330/posterization.fs",
                                "glsl330/dream_vision.fs",
                                "glsl330/pixelizer.fs",
                                "glsl330/cross_hatching.fs",
                                "glsl330/cross_stitching.fs",
                                "glsl330/predator.fs",
                                "glsl330/scanlines.fs",
                                "glsl330/fisheye.fs",
                                "glsl330/sobel.fs",
                                "glsl330/bloom.fs",
                                "glsl330/blur.fs",
                                "glsl330/horror.fs",
                                "glsl330/brainrot.fs",
                                "glsl330/fog.fs",
                                "glsl330/fxaa.fs",
                                "glsl330/cel_fxaa.fs"};

  for (int i = 0; i < 17; i++) {
    std::vector<char> shader_data = assetLoader.getAssetData(shader_names[i]);

    if (shader_data.size() > 0) {
      shader[i] = LoadShaderFromMemory(0, shader_data.data());
    } else {
      std::cout << "SCAM ALERT: Missing shader -> " << shader_names[i]
                << std::endl;
      shader[i] = shader[0];
    }
  }

  timeLoc = GetShaderLocation(shader[FX_BRAINROT], "time");
}

PostProcessing::~PostProcessing() {
  for (int i = 0; i < 16; i++) {
    UnloadShader(shader[i]);
  }
}

Shader PostProcessing::GetShader(PostproShader shaderType) {
  return shader[shaderType];
}

Shader PostProcessing::GetShader(const char *shaderName) {
  for (int i = 0; i < 16; i++) {
    if (strcmp(shaderName, postproShaderText[i]) == 0) {
      return shader[i];
    }
  }
  return shader[FX_GRAYSCALE];
}

void PostProcessing::SetShader(PostproShader shaderType) {
  currentShader = shaderType;
}

void PostProcessing::SetShader(const char *shaderName) {
  for (int i = 0; i < 16; i++) {
    if (strcmp(shaderName, postproShaderText[i]) == 0) {
      currentShader = (PostproShader)i;
      return;
    }
  }
}

Shader PostProcessing::GetCurrentShader() { return shader[currentShader]; }

int PostProcessing::GetCurrentShaderIndex() { return currentShader; }
