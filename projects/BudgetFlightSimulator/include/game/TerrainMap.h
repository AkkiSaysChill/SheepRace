#pragma once
#include "raylib.h"
#include "raymath.h"
#include <string>
#include <unordered_map>
#include <vector>

class Engine;

struct MapPropInstance {
  std::string modelKey;
  Matrix transform;
  Vector3 position;
  float boundingRadius;
};

class TerrainMap {
public:
  TerrainMap();
  ~TerrainMap();

  void LoadWorld(Engine *engine, const std::string &glbFilename);
  void Draw(Shader lightingShader);
  void DrawProps(Camera3D camera);
  void Unload();

  float GetHeightAt(float worldX, float worldZ) const;

private:
  void LoadAssetBundles(Engine *engine);
  void ParseBuildingLocations(const void *data, size_t size);

  Engine *m_engine = nullptr;

  // Terrain Model
  Model m_terrainModel = {0};
  bool m_terrainLoaded = false;

  // Road Model
  Model m_roadModel = {0};
  bool m_roadLoaded = false;

  // Instanced Props & Master Models
  std::unordered_map<std::string, Model> m_masterModels;
  std::vector<std::string> m_modelsHouses;
  std::vector<MapPropInstance> m_instances;
};
