#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

struct MapPropInstance {
    Model* parentModel;
    int meshIndex;
    Matrix transform;
    Vector3 position;
    float boundingRadius;
};

class TerrainMap {
public:
    void LoadWorld(class Engine* engine, const char* mapName = nullptr);
    void Draw(Shader shader);
    void DrawProps(Camera3D camera);
    void Unload();

    float GetHeightAt(float worldX, float worldZ) const;
    Vector3 GetNormalAt(float worldX, float worldZ) const;
    bool IsInRunwayZone(float worldX, float worldZ) const;

private:
    void BuildIslandMesh(Model& outModel, Color tintColor, float centerX, float centerZ, float radius, int resolution, bool isMountain);
    void GenerateThreeIslands();
    void LoadAssetBundles(class Engine* engine);
    void PopulateThreeIslands();

    // Island Models
    Model m_cityIslandModel = { 0 };
    Model m_mountainIslandModel = { 0 };
    Model m_megaIslandModel = { 0 };
    Model m_runwayModel = { 0 };
    bool m_terrainsGenerated = false;

    // Prop Packs
    Model m_housesModel = { 0 };
    Model m_natureModel = { 0 };
    bool m_housesLoaded = false;
    bool m_natureLoaded = false;

    // Prop Collections
    std::vector<MapPropInstance> m_houseInstances;
    std::vector<MapPropInstance> m_natureInstances;

    // PRNG
    unsigned int m_seed = 987654;
    float RandomFloat(float min, float max);
    int RandomInt(int min, int max);

    // Mesh indexing for nature_pack.glb
    const int MESH_DECIDUOUS_START = 0, MESH_DECIDUOUS_COUNT = 3;
    const int MESH_CONIFEROUS_START = 3, MESH_CONIFEROUS_COUNT = 4;
    const int MESH_BUSH_START = 7,       MESH_BUSH_COUNT = 4;
    const int MESH_LOG_START = 11,       MESH_LOG_COUNT = 2;
    const int MESH_STONE_START = 13,     MESH_STONE_COUNT = 4;
    Texture2D m_houseTexture = { 0 };
    Texture2D m_natureTexture = { 0 };
};