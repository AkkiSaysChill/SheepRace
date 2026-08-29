#pragma once

#include "asset_loader.hpp"
#include "raylib.h"

typedef enum {
  FX_GRAYSCALE = 0,
  FX_POSTERIZATION,
  FX_DREAM_VISION,
  FX_PIXELIZER,
  FX_CROSS_HATCHING,
  FX_CROSS_STITCHING,
  FX_PREDATOR_VIEW,
  FX_SCANLINES,
  FX_FISHEYE,
  FX_SOBEL,
  FX_BLOOM,
  FX_BLUR,
  FX_HORROR,
  FX_BRAINROT,
  // FX_FXAA
} PostproShader;

static const char *postproShaderText[] = {
    "GRAYSCALE",      "POSTERIZATION",   "DREAM_VISION",  "PIXELIZER",
    "CROSS_HATCHING", "CROSS_STITCHING", "PREDATOR_VIEW", "SCANLINES",
    "FISHEYE",        "SOBEL",           "BLOOM",         "BLUR",
    "HORROR",         "BRAINROT",
    //"FXAA"
};

class PostProcessing {
public:
  PostProcessing(AssetPacking::AssetLoader &assetLoader);
  ~PostProcessing();

  Shader GetShader(PostproShader shaderType);
  Shader GetShader(const char *shaderName);
  void SetShader(PostproShader shaderType);
  void SetShader(const char *shaderName);
  Shader GetCurrentShader();
  int GetCurrentShaderIndex();

  int timeLoc;

private:
  Shader shader[14] = {0};
  PostproShader currentShader;
};
