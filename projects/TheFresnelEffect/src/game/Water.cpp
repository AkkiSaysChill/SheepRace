#include "game/Water.h"
#include "engine/DrawableComponent.h"
#include "engine/Engine.h"
#include "engine/asset_loader.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <iostream>

Water::Water(Engine *engine, Vector3 pos, Vector2 size) : m_engine(engine) {
  this->name = "Water Body";
  this->position = pos;

  Mesh planeMesh = GenMeshPlane(size.x, size.y, 64, 64);
  m_waterModel = LoadModelFromMesh(planeMesh);

  auto &assetLoader = m_engine->GetAssetLoader();
  std::vector<char> vsData = assetLoader.getAssetData("glsl330/fresnel.vs");
  std::vector<char> fsData = assetLoader.getAssetData("glsl330/fresnel.fs");

  if (vsData.size() > 0 && fsData.size() > 0) {
    SaveFileData("temp_fresnel.vs", vsData.data(), vsData.size());
    SaveFileData("temp_fresnel.fs", fsData.data(), fsData.size());
    m_fresnelShader = LoadShader("temp_fresnel.vs", "temp_fresnel.fs");
    remove("temp_fresnel.vs");
    remove("temp_fresnel.fs");
  } else {
    std::cout << "ERROR: missing fresnel shaders" << std::endl;
    m_fresnelShader = LoadShader(0, 0);
  }

  m_cameraPosLoc = GetShaderLocation(m_fresnelShader, "cameraPos");
  m_scrollLoc = GetShaderLocation(m_fresnelShader, "scroll");
  m_nearLoc = GetShaderLocation(m_fresnelShader, "cameraNear");
  m_farLoc = GetShaderLocation(m_fresnelShader, "cameraFar");

  // Cache the unique uniform location for SSR ray marching calculations
  m_mvpLoc = GetShaderLocation(m_fresnelShader, "u_mvp");

  int r0Loc = GetShaderLocation(m_fresnelShader, "r0");
  float r0 = 0.02f;
  SetShaderValue(m_fresnelShader, r0Loc, &r0, SHADER_UNIFORM_FLOAT);

  m_waterModel.materials[0].shader = m_fresnelShader;

  // Bind Depth Map sampler to Roughness Slot
  m_fresnelShader.locs[SHADER_LOC_MAP_ROUGHNESS] =
      GetShaderLocation(m_fresnelShader, "depthMap");
  m_waterModel.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture =
      m_engine->GetRenderer().GetTarget().depth;

  m_waterModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color.a = 254;

  // Bind Reflection Map sampler to Emission Slot
  m_fresnelShader.locs[SHADER_LOC_MAP_EMISSION] =
      GetShaderLocation(m_fresnelShader, "reflectionMap");
  m_waterModel.materials[0].maps[MATERIAL_MAP_EMISSION].texture =
      m_engine->GetRenderer().GetReflectionTarget().texture;

  AddComponent<DrawableComponent>(m_waterModel);
}

Water::~Water() { UnloadModel(m_waterModel); }

void Water::Update(float deltaTime) {
  GameObject::Update(deltaTime);
  m_time += deltaTime;

  Camera &camera = m_engine->GetCamera();

  float camPos[3] = {camera.position.x, camera.position.y, camera.position.z};
  SetShaderValue(m_fresnelShader, m_cameraPosLoc, camPos, SHADER_UNIFORM_VEC3);
  SetShaderValue(m_fresnelShader, m_scrollLoc, &m_time, SHADER_UNIFORM_FLOAT);

  float nearPlane = 0.01f;
  float farPlane = 1000.0f;
  SetShaderValue(m_fresnelShader, m_nearLoc, &nearPlane, SHADER_UNIFORM_FLOAT);
  SetShaderValue(m_fresnelShader, m_farLoc, &farPlane, SHADER_UNIFORM_FLOAT);

  // Keep frame buffers locked for execution
  m_waterModel.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture =
      m_engine->GetRenderer().GetTarget().depth;
  m_waterModel.materials[0].maps[MATERIAL_MAP_EMISSION].texture =
      m_engine->GetRenderer().GetReflectionTarget().texture;
}
