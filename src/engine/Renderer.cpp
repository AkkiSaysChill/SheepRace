#include "engine/Renderer.h"
#include "engine/DrawableComponent.h"
#include "engine/GameObjectManager.h"
#include "engine/asset_loader.hpp"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#define SHADOWMAP_RESOLUTION 4096

RenderTexture2D LoadShadowmapRenderTexture(int width, int height);
Image GenImageWhiteNoise(int width, int height);

RenderTexture2D LoadDepthRenderTexture(int width, int height) {
  RenderTexture2D target = {0};
  target.id = rlLoadFramebuffer();
  if (target.id > 0) {
    rlEnableFramebuffer(target.id);

    // 1. Color Texture
    target.texture.id =
        rlLoadTexture(0, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    target.texture.width = width;
    target.texture.height = height;
    target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    target.texture.mipmaps = 1;
    rlFramebufferAttach(target.id, target.texture.id,
                        RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D,
                        0);

    // 2. Depth Texture
    target.depth.id = rlLoadTextureDepth(width, height, false);
    target.depth.width = width;
    target.depth.height = height;
    target.depth.format = 19; // DEPTH_COMPONENT_24
    target.depth.mipmaps = 1;
    rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH,
                        RL_ATTACHMENT_TEXTURE2D, 0);

    if (rlFramebufferComplete(target.id))
      TRACELOG(LOG_INFO,
               "FBO: Main Target created with readable Depth Texture");
    rlDisableFramebuffer();
  }
  return target;
}

Renderer::Renderer(int screenWidth, int screenHeight,
                   AssetPacking::AssetLoader &assetLoader) {
  std::vector<char> shadowShaderVS =
      assetLoader.getAssetData("glsl330/shadowmap.vs");
  std::vector<char> shadowShaderFS =
      assetLoader.getAssetData("glsl330/shadowmap.fs");
  shadowShaderVS.push_back('\0');
  shadowShaderFS.push_back('\0');
  m_shadowShader =
      LoadShaderFromMemory(shadowShaderVS.data(), shadowShaderFS.data());

  m_reflectionTarget = LoadRenderTexture(screenWidth, screenHeight);

  m_shadowShader.locs[SHADER_LOC_VECTOR_VIEW] =
      GetShaderLocation(m_shadowShader, "viewPos");

  int shadowMapResolution = SHADOWMAP_RESOLUTION;
  SetShaderValue(m_shadowShader,
                 GetShaderLocation(m_shadowShader, "shadowMapResolution"),
                 &shadowMapResolution, SHADER_UNIFORM_INT);

  float ambient[4] = {0.1f, 0.1f, 0.1f, 1.0f};
  SetShaderValue(m_shadowShader, GetShaderLocation(m_shadowShader, "ambient"),
                 ambient, SHADER_UNIFORM_VEC4);

  int shadowBiasLoc = GetShaderLocation(m_shadowShader, "shadowBias");
  float shadowBias = 0.005f;
  SetShaderValue(m_shadowShader, shadowBiasLoc, &shadowBias,
                 SHADER_UNIFORM_FLOAT);

  m_shadowMap =
      LoadShadowmapRenderTexture(SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);

  Image noiseImage = GenImageWhiteNoise(64, 64);
  m_noiseTexture = LoadTextureFromImage(noiseImage);
  UnloadImage(noiseImage);
  SetTextureWrap(m_noiseTexture, TEXTURE_WRAP_REPEAT);

  std::vector<char> skyShaderFS = assetLoader.getAssetData("sky/sky.fs");
  skyShaderFS.push_back('\0');
  m_skyShader = LoadShaderFromMemory(0, skyShaderFS.data());

  m_target = LoadDepthRenderTexture(screenWidth, screenHeight);
}

Renderer::~Renderer() {
  UnloadShader(m_shadowShader);
  UnloadRenderTexture(m_shadowMap);
  UnloadTexture(m_noiseTexture);
  UnloadShader(m_skyShader);
  UnloadRenderTexture(m_target);
}

void Renderer::Render(GameObjectManager &scene, Camera &camera,
                      PostProcessing &postProcessing, Vector3 &lightDir,
                      bool postProcessingActive, float fov, float shininess,
                      float specularStrength, float cloudCover,
                      float cloudFalloff, float lightCamOrthoSize,
                      Draw3DCallback extraDraw3D) {
  camera.fovy = fov;
  lightDir = Vector3Normalize(lightDir);

  int lightDirLoc = GetShaderLocation(m_shadowShader, "lightDir");
  SetShaderValue(m_shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);

  Vector3 cameraPos = camera.position;
  SetShaderValue(m_shadowShader, m_shadowShader.locs[SHADER_LOC_VECTOR_VIEW],
                 &cameraPos, SHADER_UNIFORM_VEC3);

  m_currentSkySunVector = Vector3Negate(lightDir);
  SetShaderValue(m_skyShader, GetShaderLocation(m_skyShader, "u_lightDir"),
                 &m_currentSkySunVector, SHADER_UNIFORM_VEC3);

  // ── 1. SHADOW MAP PASS ───────────────────────────────────────
  Matrix lightView, lightProj;
  Camera3D lightCam = {0};
  lightCam.target = camera.target;
  lightCam.position =
      Vector3Add(lightCam.target, Vector3Scale(lightDir, -15.0f));
  lightCam.projection = CAMERA_ORTHOGRAPHIC;
  lightCam.up = {0.0f, 1.0f, 0.0f};
  lightCam.fovy = lightCamOrthoSize;

  BeginTextureMode(m_shadowMap);
  rlColorMask(false, false, false, false);
  ClearBackground(WHITE);
  BeginMode3D(lightCam);
  lightView = rlGetMatrixModelview();
  lightProj = rlGetMatrixProjection();
  scene.Draw();
  EndMode3D();
  EndTextureMode();
  rlColorMask(true, true, true, true);

  Matrix lightViewProj = MatrixMultiply(lightView, lightProj);

  // ── 2. SKYBOX DATA UPDATES ───────────────────────────────────
  float time = (float)GetTime();
  float currentScreenWidth = (float)GetScreenWidth();
  float currentScreenHeight = (float)GetScreenHeight();
  float resolutionValues[2] = {currentScreenWidth, currentScreenHeight};

  Vector3 camForward =
      Vector3Normalize(Vector3Subtract(camera.target, camera.position));
  Vector3 camUp = Vector3Normalize(camera.up);
  Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, camUp));
  camUp = Vector3Normalize(Vector3CrossProduct(camRight, camForward));

  SetShaderValue(m_skyShader, GetShaderLocation(m_skyShader, "iTime"), &time,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(m_skyShader, GetShaderLocation(m_skyShader, "iResolution"),
                 resolutionValues, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(
      m_skyShader, GetShaderLocation(m_skyShader, "iChannel0"), m_noiseTexture);
  Vector3 skySunVector = Vector3Negate(lightDir);
  SetShaderValue(m_skyShader, GetShaderLocation(m_skyShader, "u_lightDir"),
                 &skySunVector, SHADER_UNIFORM_VEC3);
  SetShaderValue(m_skyShader, GetShaderLocation(m_skyShader, "u_cloudCover"),
                 &cloudCover, SHADER_UNIFORM_FLOAT);
  SetShaderValue(m_skyShader, GetShaderLocation(m_skyShader, "u_cloudFalloff"),
                 &cloudFalloff, SHADER_UNIFORM_FLOAT);
  SetShaderValue(m_skyShader, GetShaderLocation(m_skyShader, "u_cameraPos"),
                 &camera.position, SHADER_UNIFORM_VEC3);
  SetShaderValue(m_skyShader, GetShaderLocation(m_skyShader, "u_cameraForward"),
                 &camForward, SHADER_UNIFORM_VEC3);
  SetShaderValue(m_skyShader, GetShaderLocation(m_skyShader, "u_cameraUp"),
                 &camUp, SHADER_UNIFORM_VEC3);
  SetShaderValue(m_skyShader, GetShaderLocation(m_skyShader, "u_cameraRight"),
                 &camRight, SHADER_UNIFORM_VEC3);
  float fovYRadians = camera.fovy * DEG2RAD;
  SetShaderValue(m_skyShader, GetShaderLocation(m_skyShader, "u_cameraFovY"),
                 &fovYRadians, SHADER_UNIFORM_FLOAT);

  // ── 3. PLANAR REFLECTION PASS ────────────────────────────────
  if (extraDraw3D) {
    Camera reflectionCam = camera;
    reflectionCam.position.y = -camera.position.y;
    reflectionCam.target.y = -camera.target.y;

    Vector3 rwd = Vector3Normalize(
        Vector3Subtract(reflectionCam.target, reflectionCam.position));
    Vector3 rRight = Vector3Normalize(Vector3CrossProduct(rwd, camera.up));
    reflectionCam.up = Vector3Normalize(Vector3CrossProduct(rRight, rwd));

    BeginTextureMode(m_reflectionTarget);
    ClearBackground(BLACK);

    BeginShaderMode(m_skyShader);
    DrawRectangle(0, 0, m_reflectionTarget.texture.width,
                  m_reflectionTarget.texture.height, WHITE);
    EndShaderMode();

    rlPushMatrix();
    BeginMode3D(reflectionCam);

    int shininessLoc = GetShaderLocation(m_shadowShader, "u_shininess");
    int specStrengthLoc =
        GetShaderLocation(m_shadowShader, "u_specularStrength");
    int lightVPLoc = GetShaderLocation(m_shadowShader, "lightVP");
    int shadowMapLoc = GetShaderLocation(m_shadowShader, "shadowMap");

    SetShaderValueMatrix(m_shadowShader, lightVPLoc, lightViewProj);
    SetShaderValue(m_shadowShader, shininessLoc, &shininess,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shadowShader, specStrengthLoc, &specularStrength,
                   SHADER_UNIFORM_FLOAT);

    rlEnableShader(m_shadowShader.id);
    int slot = 10;
    rlActiveTextureSlot(10);
    rlEnableTexture(m_shadowMap.depth.id);
    rlSetUniform(shadowMapLoc, &slot, SHADER_UNIFORM_INT, 1);

    for (const auto &obj : scene.GetGameObjects()) {
      if (obj->GetComponent<DrawableComponent>()) {
        if (obj->name.find("STATIC_TREE_") != std::string::npos)
          continue;
        if (obj->position.y < -0.1f &&
            obj->name.find("underwater_") != std::string::npos)
          continue;
        obj->GetComponent<DrawableComponent>()->Draw();
      }
    }

    EndMode3D();
    rlPopMatrix();
    EndTextureMode();
  }

  // ── 4. MAIN OPAQUE WORLD PASS ────────────────────────────────
  BeginTextureMode(m_target);
  ClearBackground(RAYWHITE);

  BeginShaderMode(m_skyShader);
  DrawRectangle(0, 0, (int)currentScreenWidth, (int)currentScreenHeight, WHITE);
  EndShaderMode();

  rlPushMatrix();
  BeginMode3D(camera);
  int shininessLoc = GetShaderLocation(m_shadowShader, "u_shininess");
  int specStrengthLoc = GetShaderLocation(m_shadowShader, "u_specularStrength");
  int lightVPLoc = GetShaderLocation(m_shadowShader, "lightVP");
  int shadowMapLoc = GetShaderLocation(m_shadowShader, "shadowMap");

  SetShaderValueMatrix(m_shadowShader, lightVPLoc, lightViewProj);
  SetShaderValue(m_shadowShader, shininessLoc, &shininess,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(m_shadowShader, specStrengthLoc, &specularStrength,
                 SHADER_UNIFORM_FLOAT);

  rlEnableShader(m_shadowShader.id);
  int slot = 10;
  rlActiveTextureSlot(10);
  rlEnableTexture(m_shadowMap.depth.id);
  rlSetUniform(shadowMapLoc, &slot, SHADER_UNIFORM_INT, 1);

  scene.Draw();
  EndMode3D();
  rlPopMatrix();
  EndTextureMode();

  // ── 5. TRANSPARENT PASS ──────────────────────────────────────
  if (extraDraw3D) {
    BeginTextureMode(m_target);
    rlPushMatrix();
    BeginMode3D(camera);

    rlDisableDepthMask();
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE,
                              RL_ONE, RL_FUNC_ADD, RL_MAX);

    extraDraw3D();

    EndBlendMode();
    rlEnableDepthMask();

    EndMode3D();
    rlPopMatrix();
    EndTextureMode();
  }

  // ── 6. FINAL SCREEN COMPOSITION ──────────────────────────────
  BeginDrawing();
  if (postProcessingActive) {
    Shader activeShader = postProcessing.GetCurrentShader();
    int loc = GetShaderLocation(activeShader, "time");
    if (loc != -1)
      SetShaderValue(activeShader, loc, &time, SHADER_UNIFORM_FLOAT);

    // Bind depth texture to slot 1 for shaders that ask for depth (such as fog)
    int depthLoc = GetShaderLocation(activeShader, "depthTexture");
    if (depthLoc != -1) {
      SetShaderValueTexture(activeShader, depthLoc, m_target.depth);
    }

    BeginShaderMode(activeShader);
    DrawTextureRec(m_target.texture,
                   Rectangle{0, 0, (float)m_target.texture.width,
                             (float)-m_target.texture.height},
                   Vector2{0, 0}, WHITE);
    EndShaderMode();
  } else {
    DrawTextureRec(m_target.texture,
                   Rectangle{0, 0, (float)m_target.texture.width,
                             (float)-m_target.texture.height},
                   Vector2{0, 0}, WHITE);
  }
}

void Renderer::OnResize(int width, int height) {
  UnloadRenderTexture(m_target);
  UnloadRenderTexture(m_reflectionTarget);
  m_target = LoadDepthRenderTexture(width, height);
  m_reflectionTarget = LoadRenderTexture(width, height);
}

Shader &Renderer::GetShadowShader() { return m_shadowShader; }

RenderTexture2D LoadShadowmapRenderTexture(int width, int height) {
  RenderTexture2D target = {0};
  target.id = rlLoadFramebuffer();
  target.texture.width = width;
  target.texture.height = height;

  if (target.id > 0) {
    rlEnableFramebuffer(target.id);
    target.depth.id = rlLoadTextureDepth(width, height, false);
    target.depth.width = width;
    target.depth.height = height;
    target.depth.format = 19;
    target.depth.mipmaps = 1;
    rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH,
                        RL_ATTACHMENT_TEXTURE2D, 0);
    if (rlFramebufferComplete(target.id))
      TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully",
               target.id);
    rlDisableFramebuffer();
  } else {
    TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");
  }
  return target;
}

Image GenImageWhiteNoise(int width, int height) {
  Color *pixels = (Color *)malloc(width * height * sizeof(Color));
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      unsigned char val = (unsigned char)(rand() % 255);
      pixels[y * width + x] = {val, val, val, 255};
    }
  }
  return Image{pixels, width, height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
}
