#pragma once

#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>

class Engine;

class TerrainMap {
public:
  TerrainMap() = default;
  ~TerrainMap() { Unload(); }

  void LoadWorld(Engine *engine, const char *mapName = "island_world.glb");
  void Draw(Shader shader);
  void DrawProps(Camera3D camera);
  void Unload();

  float GetHeightAt(float worldX, float worldZ) const;
  Vector3 GetNormalAt(float worldX, float worldZ) const;
  bool IsInRunwayZone(float worldX, float worldZ) const;

  void DumpCalculatedLandmarks() const;

private:
  void LoadAssetBundles(Engine *engine);
  void GenerateFourIslands();
  void PopulateIslands();

  void BuildIslandMesh(Model &outModel, Color tintColor, float centerX,
                       float centerZ, float radius, int resolution,
                       bool isMountain);

  float RandomFloat(float min, float max);
  int RandomInt(int min, int max);

  unsigned int m_seed = 987654;

  // Island 3D Models
  Model m_cityIslandModel = {0};
  Model m_mountainIslandModel = {0};
  Model m_megaIslandModel = {0};
  Model m_southIslandModel = {0};

  // Runway Meshes
  Model m_runwayModel = {0};
  Model m_southRunwayModel = {0};
  bool m_terrainsGenerated = false;

  // Prop Models & Textures
  Model m_housesModel = {0};
  Model m_natureModel = {0};
  Texture2D m_houseTexture = {0};
  Texture2D m_natureTexture = {0};
  bool m_housesLoaded = false;
  bool m_natureLoaded = false;

  struct PropInstance {
    Model *modelRef;
    int meshIndex;
    Matrix transform;
    Vector3 position;
    float boundingRadius;
  };

  std::vector<PropInstance> m_houseInstances;
  std::vector<PropInstance> m_natureInstances;

  // Prop Mesh Index Ranges in nature_pack.glb
  static const int MESH_DECIDUOUS_START = 0;
  static const int MESH_DECIDUOUS_COUNT = 6;
  static const int MESH_CONIFEROUS_START = 6;
  static const int MESH_CONIFEROUS_COUNT = 6;
  static const int MESH_BUSH_START = 12;
  static const int MESH_BUSH_COUNT = 4;
  static const int MESH_STONE_START = 16;
  static const int MESH_STONE_COUNT = 4;
};