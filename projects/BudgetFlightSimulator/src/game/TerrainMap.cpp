#include "game/TerrainMap.h"
#include "engine/Engine.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "external/cgltf.h"

TerrainMap::TerrainMap()
    : m_engine(nullptr), m_terrainLoaded(false), m_roadLoaded(false) {}

TerrainMap::~TerrainMap() { Unload(); }

void TerrainMap::Unload() {
  if (m_terrainLoaded) {
    UnloadModel(m_terrainModel);
    m_terrainLoaded = false;
  }

  if (m_roadLoaded) {
    UnloadModel(m_roadModel);
    m_roadLoaded = false;
  }

  for (auto &pair : m_masterModels) {
    UnloadModel(pair.second);
  }
  m_masterModels.clear();
  m_modelsHouses.clear();
  m_instances.clear();
}

void TerrainMap::LoadAssetBundles(Engine *engine) {
  auto &assetLoader = engine->GetAssetLoader();
  auto &renderer = engine->GetRenderer();
  Shader shadowShader = renderer.GetShadowShader();

  assetLoader.loadPackedFile("houses.bin");

  // Standard low-poly houses (excluding huge landmarks like windmills)
  m_modelsHouses = {"low_poly_house_1.glb", "low_poly_house_2.glb",
                    "low_poly_house_3.glb", "low_poly_house_4.glb",
                    "low_poly_house_5.glb", "barn_low_poly.glb"};

  for (const auto &file : m_modelsHouses) {
    if (m_masterModels.find(file) != m_masterModels.end())
      continue;

    auto data = assetLoader.getAssetData(file);
    if (!data.empty()) {
      Model model = assetLoader.LoadModelFromAssetData(data, ".glb");
      for (int i = 0; i < model.materialCount; ++i) {
        model.materials[i].shader = shadowShader;
      }
      m_masterModels[file] = model;
    }
  }
}

float TerrainMap::GetHeightAt(float worldX, float worldZ) const {
  if (!m_terrainLoaded || m_terrainModel.meshCount == 0)
    return 0.0f;

  // Raycast straight down from a high altitude
  Ray ray = {{worldX, 4000.0f, worldZ}, {0.0f, -1.0f, 0.0f}};
  RayCollision col = GetRayCollisionMesh(ray, m_terrainModel.meshes[0],
                                         m_terrainModel.transform);

  if (col.hit)
    return col.point.y;

  return 0.0f;
}

void TerrainMap::ParseBuildingLocations(const void *data, size_t size) {
  if (m_modelsHouses.empty() || !data || size == 0)
    return;

  cgltf_options options = {};
  cgltf_data *gltfData = nullptr;

  if (cgltf_parse(&options, data, size, &gltfData) != cgltf_result_success)
    return;

  if (cgltf_load_buffers(&options, gltfData, nullptr) != cgltf_result_success) {
    cgltf_free(gltfData);
    return;
  }

  srand(1337); // Seed ensures consistent placement every run

  const float minDistanceSq =
      25.0f * 25.0f; // Minimum 25 meters spacing between buildings
  const size_t maxTotalBuildings = 250; // Safety cap to maintain solid 60+ FPS

  for (cgltf_size m = 0; m < gltfData->meshes_count; ++m) {
    cgltf_mesh *mesh = &gltfData->meshes[m];
    for (cgltf_size p = 0; p < mesh->primitives_count; ++p) {
      cgltf_primitive *prim = &mesh->primitives[p];

      for (cgltf_size a = 0; a < prim->attributes_count; ++a) {
        if (prim->attributes[a].type == cgltf_attribute_type_position) {
          cgltf_accessor *acc = prim->attributes[a].data;

          // Step across vertices to prevent multi-point polygon clustering
          cgltf_size step = (acc->count > 500) ? 12 : 4;
          for (cgltf_size v = 0; v < acc->count; v += step) {
            if (m_instances.size() >= maxTotalBuildings)
              break;

            float pos[3] = {0.0f, 0.0f, 0.0f};
            cgltf_accessor_read_float(acc, v, pos, 3);

            float x = pos[0];
            float z = pos[2];

            // Skip near-origin cluster artifacts
            if (fabsf(x) < 1.0f && fabsf(z) < 1.0f)
              continue;

            // Raycast to find terrain elevation
            float y = GetHeightAt(x, z);
            if (y <= 2.0f) // Ignore sea-level or invalid hits
              continue;

            Vector3 candidatePos = {x, y, z};

            // Check distance against already spawned houses
            bool tooClose = false;
            for (const auto &existing : m_instances) {
              if (Vector3DistanceSqr(candidatePos, existing.position) <
                  minDistanceSq) {
                tooClose = true;
                break;
              }
            }
            if (tooClose)
              continue;

            std::string model = m_modelsHouses[rand() % m_modelsHouses.size()];
            float rot = (float)(rand() % 4) * 90.0f;

            Matrix transform = MatrixMultiply(MatrixRotateY(rot * DEG2RAD),
                                              MatrixTranslate(x, y, z));

            m_instances.push_back({model, transform, candidatePos, 8.0f});
          }
        }
      }
    }
  }

  cgltf_free(gltfData);
}

void TerrainMap::LoadWorld(Engine *engine, const std::string &glbFilename) {
  Unload();
  m_engine = engine;

  LoadAssetBundles(engine);

  auto &assetLoader = engine->GetAssetLoader();
  assetLoader.loadPackedFile("map.bin");

  // 1. Load Main Island Elevation Mesh
  auto worldData = assetLoader.getAssetData("island_world.glb");
  if (!worldData.empty()) {
    m_terrainModel = assetLoader.LoadModelFromAssetData(worldData, ".glb");
    if (m_terrainModel.meshCount > 0) {
      for (int i = 0; i < m_terrainModel.materialCount; ++i) {
        m_terrainModel.materials[i].shader =
            engine->GetRenderer().GetShadowShader();
      }
      m_terrainLoaded = true;
    }
  } else {
    TraceLog(LOG_ERROR, "TERRAIN: Could not find island_world.glb in map.bin");
  }

  // 2. Load Road Mesh
  auto roadData = assetLoader.getAssetData("island_road.glb");
  if (!roadData.empty()) {
    m_roadModel = assetLoader.LoadModelFromAssetData(roadData, ".glb");
    if (m_roadModel.meshCount > 0) {
      for (int i = 0; i < m_roadModel.materialCount; ++i) {
        m_roadModel.materials[i].shader =
            engine->GetRenderer().GetShadowShader();
      }
      m_roadLoaded = true;
    }
  }

  // 3. Read Footprints and Instantiate Buildings
  auto buildingLocData = assetLoader.getAssetData("island_buildingLoc.glb");
  if (!buildingLocData.empty()) {
    ParseBuildingLocations(buildingLocData.data(), buildingLocData.size());
  }
}

void TerrainMap::Draw(Shader lightingShader) {
  if (m_terrainLoaded) {
    for (int i = 0; i < m_terrainModel.materialCount; ++i) {
      m_terrainModel.materials[i].shader = lightingShader;
    }
    DrawModel(m_terrainModel, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
  }

  if (m_roadLoaded) {
    for (int i = 0; i < m_roadModel.materialCount; ++i) {
      m_roadModel.materials[i].shader = lightingShader;
    }
    // Offset Y by +0.15m to completely prevent Z-fighting flicker against
    // terrain
    DrawModel(m_roadModel, {0.0f, 0.15f, 0.0f}, 1.0f, DARKGRAY);
  }
}

void TerrainMap::DrawProps(Camera3D camera) {
  const float maxDistSq = 5000.0f * 5000.0f; // 5km render culling radius

  for (const auto &inst : m_instances) {
    float distSq = Vector3DistanceSqr(camera.position, inst.position);
    if (distSq > maxDistSq)
      continue;

    auto it = m_masterModels.find(inst.modelKey);
    if (it != m_masterModels.end()) {
      Model model = it->second;
      model.transform = inst.transform;
      DrawModel(model, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    }
  }
}
