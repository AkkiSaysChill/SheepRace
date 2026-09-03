#include "game/TerrainMap.h"
#include "engine/Engine.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// =========================================================================
// DETERMINISTIC PRNG
// =========================================================================

float TerrainMap::RandomFloat(float min, float max) {
    m_seed = (m_seed * 1664525u + 1013904223u);
    float t = (float)(m_seed & 0x00FFFFFF) / (float)0x00FFFFFF;
    return min + t * (max - min);
}

int TerrainMap::RandomInt(int min, int max) {
    if (min >= max) return min;
    m_seed = (m_seed * 1664525u + 1013904223u);
    return min + (int)((m_seed & 0x7FFFFFFF) % (unsigned int)(max - min + 1));
}

// =========================================================================
// ISLAND & RUNWAY CONSTANTS
// =========================================================================

// 1. West City Island
static const Vector2 kCityCenter = { -2000.0f, 0.0f };
static const float kCityRadius = 1350.0f;

// 2. East Mountain Island
static const Vector2 kMountainCenter = { 2000.0f, 0.0f };
static const float kMountainRadius = 1400.0f;

// 3. North Mega Island
static const Vector2 kMegaCenter = { 0.0f, 5500.0f };
static const float kMegaRadius = 2400.0f;

// Runway on West Island
static const float kRunwayCenterX = -2000.0f;
static const float kRunwayCenterZ = 150.0f;
static const float kRunwayHalfLen = 450.0f;
static const float kRunwayHalfWid = 35.0f;
static const float kRunwayElev = 110.0f;

bool TerrainMap::IsInRunwayZone(float worldX, float worldZ) const {
    return (fabsf(worldX - kRunwayCenterX) <= (kRunwayHalfLen + 50.0f) &&
            fabsf(worldZ - kRunwayCenterZ) <= (kRunwayHalfWid + 50.0f));
}

// =========================================================================
// SMOOTH ORGANIC HEIGHTMAP
// =========================================================================

float TerrainMap::GetHeightAt(float worldX, float worldZ) const {
    float height = 0.0f;

    // 1. West City Island
    float dxC = worldX - kCityCenter.x;
    float dzC = worldZ - kCityCenter.y;
    float distCity = sqrtf(dxC * dxC + dzC * dzC);

    float angleCity = atan2f(dzC, dxC);
    float coastlineVariation = sinf(angleCity * 3.0f + 1.2f) * 160.0f
                             + sinf(angleCity * 7.0f) * 75.0f
                             + cosf(angleCity * 13.0f) * 35.0f;
    float effectiveCityRadius = kCityRadius + coastlineVariation;

    if (distCity < effectiveCityRadius) {
        float norm = distCity / effectiveCityRadius;
        float baseLand = powf(fmaxf(0.0f, 1.0f - norm), 0.50f) * 135.0f;

        float shoreFade = sinf(norm * 3.14159265f);
        float n1 = sinf(worldX * 0.0031f + worldZ * 0.0017f) * cosf(worldZ * 0.0029f - worldX * 0.0013f) * 28.0f;
        float n2 = sinf(worldX * 0.0075f + 1.7f) * cosf(worldZ * 0.0068f + 0.9f) * 12.0f;
        float hills = (n1 + n2) * shoreFade;

        height = fmaxf(0.0f, baseLand + hills);

        // Runway plateau blend
        float rx = fabsf(worldX - kRunwayCenterX);
        float rz = fabsf(worldZ - kRunwayCenterZ);
        float blendX = Clamp(1.0f - (rx - kRunwayHalfLen) / 180.0f, 0.0f, 1.0f);
        float blendZ = Clamp(1.0f - (rz - kRunwayHalfWid) / 120.0f, 0.0f, 1.0f);
        float plateauWeight = blendX * blendZ;
        plateauWeight = plateauWeight * plateauWeight * (3.0f - 2.0f * plateauWeight);
        height = Lerp(height, kRunwayElev, plateauWeight);
    }

    // 2. East Mountain Island
    float dxM = worldX - kMountainCenter.x;
    float dzM = worldZ - kMountainCenter.y;
    float distMtn = sqrtf(dxM * dxM + dzM * dzM);

    float angleMtn = atan2f(dzM, dxM);
    float mountainCoastVariation = sinf(angleMtn * 4.0f) * 140.0f + cosf(angleMtn * 9.0f) * 60.0f;
    float effectiveMtnRadius = kMountainRadius + mountainCoastVariation;

    if (distMtn < effectiveMtnRadius) {
        float norm = distMtn / effectiveMtnRadius;
        float peak = powf(cosf(norm * 1.5707963f), 1.5f) * 780.0f;
        
        float m1 = sinf(worldX * 0.0028f + worldZ * 0.0018f) * cosf(worldZ * 0.0032f) * 45.0f;
        float m2 = sinf(worldX * 0.0065f + 1.1f) * cosf(worldZ * 0.0075f) * 16.0f;

        float mtnHeight = fmaxf(0.0f, peak + m1 + m2);
        if (mtnHeight > height) height = mtnHeight;
    }

    // 3. North Mega Continent Island
    float dxMega = worldX - kMegaCenter.x;
    float dzMega = worldZ - kMegaCenter.y;
    float distMega = sqrtf(dxMega * dxMega + dzMega * dzMega);

    float angleMega = atan2f(dzMega, dxMega);
    float megaCoastVariation = sinf(angleMega * 3.0f) * 220.0f
                             + cosf(angleMega * 6.0f + 0.8f) * 120.0f
                             + sinf(angleMega * 11.0f) * 50.0f;
    float effectiveMegaRadius = kMegaRadius + megaCoastVariation;

    if (distMega < effectiveMegaRadius) {
        float norm = distMega / effectiveMegaRadius;
        float baseElevation = powf(cosf(norm * 1.5707963f), 1.2f) * 140.0f;

        float cliffOffsetZ = (dzMega + 500.0f) / (kMegaRadius * 0.9f);
        float cliffFactor = Clamp(cliffOffsetZ, 0.0f, 1.0f);
        cliffFactor = cliffFactor * cliffFactor * (3.0f - 2.0f * cliffFactor);
        float cliffPeak = cliffFactor * 480.0f;

        float c1 = sinf(worldX * 0.0022f) * cosf(worldZ * 0.0026f) * 38.0f;
        float c2 = sinf(worldX * 0.008f + 1.4f) * cosf(worldZ * 0.009f) * 14.0f;

        float megaHeight = fmaxf(0.0f, baseElevation + cliffPeak + c1 + c2);
        if (megaHeight > height) height = megaHeight;
    }

    return height;
}

Vector3 TerrainMap::GetNormalAt(float worldX, float worldZ) const {
    const float step = 4.0f;
    float hL = GetHeightAt(worldX - step, worldZ);
    float hR = GetHeightAt(worldX + step, worldZ);
    float hD = GetHeightAt(worldX, worldZ - step);
    float hU = GetHeightAt(worldX, worldZ + step);

    Vector3 normal = { hL - hR, 2.0f * step, hD - hU };
    return Vector3Normalize(normal);
}

// =========================================================================
// MESH GENERATION
// =========================================================================

void TerrainMap::BuildIslandMesh(Model& outModel, Color tintColor, float centerX, float centerZ, float radius, int resolution, bool isMountain) {
    const float size = radius * 2.3f;
    const float cellSize = size / (float)resolution;
    const float halfSize = size * 0.5f;

    Mesh mesh = { 0 };
    mesh.vertexCount = (resolution + 1) * (resolution + 1);
    mesh.triangleCount = resolution * resolution * 2;

    mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.normals = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
    mesh.indices = (unsigned short*)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));

    for (int z = 0; z <= resolution; ++z) {
        for (int x = 0; x <= resolution; ++x) {
            int idx = z * (resolution + 1) + x;
            float wx = centerX - halfSize + (float)x * cellSize;
            float wz = centerZ - halfSize + (float)z * cellSize;
            float wy = GetHeightAt(wx, wz);

            mesh.vertices[idx * 3 + 0] = wx;
            mesh.vertices[idx * 3 + 1] = wy;
            mesh.vertices[idx * 3 + 2] = wz;

            Vector3 norm = GetNormalAt(wx, wz);
            mesh.normals[idx * 3 + 0] = norm.x;
            mesh.normals[idx * 3 + 1] = norm.y;
            mesh.normals[idx * 3 + 2] = norm.z;

            mesh.texcoords[idx * 2 + 0] = (float)x / (float)resolution * 16.0f;
            mesh.texcoords[idx * 2 + 1] = (float)z / (float)resolution * 16.0f;
        }
    }

    int triIdx = 0;
    for (int z = 0; z < resolution; ++z) {
        for (int x = 0; x < resolution; ++x) {
            unsigned short v0 = (unsigned short)(z * (resolution + 1) + x);
            unsigned short v1 = (unsigned short)(z * (resolution + 1) + (x + 1));
            unsigned short v2 = (unsigned short)((z + 1) * (resolution + 1) + x);
            unsigned short v3 = (unsigned short)((z + 1) * (resolution + 1) + (x + 1));

            mesh.indices[triIdx++] = v0;
            mesh.indices[triIdx++] = v2;
            mesh.indices[triIdx++] = v1;

            mesh.indices[triIdx++] = v1;
            mesh.indices[triIdx++] = v2;
            mesh.indices[triIdx++] = v3;
        }
    }

    UploadMesh(&mesh, false);
    outModel = LoadModelFromMesh(mesh);
    outModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = tintColor;
}

void TerrainMap::GenerateThreeIslands() {
    BuildIslandMesh(m_cityIslandModel, Color{ 74, 132, 68, 255 }, kCityCenter.x, kCityCenter.y, kCityRadius, 140, false);
    BuildIslandMesh(m_mountainIslandModel, Color{ 78, 102, 75, 255 }, kMountainCenter.x, kMountainCenter.y, kMountainRadius, 140, true);
    BuildIslandMesh(m_megaIslandModel, Color{ 68, 118, 72, 255 }, kMegaCenter.x, kMegaCenter.y, kMegaRadius, 180, true);

    Mesh runwayMesh = GenMeshCube(kRunwayHalfLen * 2.0f, 0.2f, kRunwayHalfWid * 2.0f);
    m_runwayModel = LoadModelFromMesh(runwayMesh);
    m_runwayModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = Color{ 38, 40, 44, 255 };

    m_terrainsGenerated = true;
}

void TerrainMap::LoadAssetBundles(Engine* engine) {
    auto& assetLoader = engine->GetAssetLoader();
    Shader shadowShader = engine->GetRenderer().GetShadowShader();

    shadowShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(shadowShader, "texture0");
    shadowShader.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(shadowShader, "colDiffuse");

    assetLoader.loadPackedFile("houses.bin");
    assetLoader.loadPackedFile("trees.bin");

    auto LoadTextureFromBundle = [&](const std::string& filename, const char* ext) -> Texture2D {
        auto rawData = assetLoader.getAssetData(filename);
        if (rawData.empty()) {
            TraceLog(LOG_ERROR, TextFormat("[TerrainMap] Packed texture not found: %s", filename.c_str()));
            return Texture2D{ 0 };
        }

        Image img = LoadImageFromMemory(ext, 
                                        reinterpret_cast<const unsigned char*>(rawData.data()), 
                                        static_cast<int>(rawData.size()));
        if (img.data == nullptr) {
            TraceLog(LOG_ERROR, TextFormat("[TerrainMap] Failed to decode image: %s", filename.c_str()));
            return Texture2D{ 0 };
        }

        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(tex, TEXTURE_FILTER_POINT);
        return tex;
    };

    m_houseTexture = LoadTextureFromBundle("color_512x512.jpg", ".jpg");
    m_natureTexture = LoadTextureFromBundle("texture_solid.png", ".png");

    auto houseData = assetLoader.getAssetData("houses_pack.glb");
    if (!houseData.empty()) {
        m_housesModel = assetLoader.LoadModelFromAssetData(houseData, ".glb");
        for (int i = 0; i < m_housesModel.materialCount; ++i) {
            m_housesModel.materials[i].shader = shadowShader;
            m_housesModel.materials[i].maps[MATERIAL_MAP_ALBEDO].texture = m_houseTexture;
            m_housesModel.materials[i].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
        }
        m_housesLoaded = (m_housesModel.meshCount > 0);
    }

    auto natureData = assetLoader.getAssetData("nature_pack.glb");
    if (!natureData.empty()) {
        m_natureModel = assetLoader.LoadModelFromAssetData(natureData, ".glb");
        for (int i = 0; i < m_natureModel.materialCount; ++i) {
            m_natureModel.materials[i].shader = shadowShader;
            m_natureModel.materials[i].maps[MATERIAL_MAP_ALBEDO].texture = m_natureTexture;
            m_natureModel.materials[i].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
        }
        m_natureLoaded = (m_natureModel.meshCount > 0);
    }
}

// =========================================================================
// DETERMINISTIC POPULATION
// =========================================================================

void TerrainMap::PopulateThreeIslands() {
    m_seed = 987654;
    m_houseInstances.clear();
    m_natureInstances.clear();

    // =========================================================================
    // 1. CITY ISLAND HOUSES (With Minimum Spacing Check)
    // =========================================================================
    if (m_housesLoaded && m_housesModel.meshCount > 0) {
        const float MIN_HOUSE_DIST_SQ = 34.0f * 34.0f; // Prevent overlap at 2.5x scale
        for (int i = 0; i < 220; ++i) {
            float dist = RandomFloat(100.0f, kCityRadius * 0.75f);
            float angle = RandomFloat(0.0f, 6.283185f);
            float x = kCityCenter.x + cosf(angle) * dist;
            float z = kCityCenter.y + sinf(angle) * dist;

            if (IsInRunwayZone(x, z)) continue;

            float y = GetHeightAt(x, z);
            if (y < 20.0f || y > 150.0f) continue;

            Vector3 norm = GetNormalAt(x, z);
            if (norm.y < 0.85f) continue;

            // Proximity rejection
            bool tooClose = false;
            for (const auto& existing : m_houseInstances) {
                float dx = existing.position.x - x;
                float dz = existing.position.z - z;
                if ((dx * dx + dz * dz) < MIN_HOUSE_DIST_SQ) {
                    tooClose = true;
                    break;
                }
            }
            if (tooClose) continue;

            int houseIdx = RandomInt(0, m_housesModel.meshCount - 1);
            float yaw = RandomFloat(0.0f, 360.0f) * DEG2RAD;

            Matrix rotScale = MatrixMultiply(MatrixScale(2.875f, 2.875f, 2.875f), MatrixRotateY(yaw));
            Matrix transform = MatrixMultiply(rotScale, MatrixTranslate(x, y, z));
            m_houseInstances.push_back({ &m_housesModel, houseIdx, transform, {x, y, z}, 20.0f });
        }
    }

    // =========================================================================
    // 2. CITY ISLAND NATURE
    // =========================================================================
    if (m_natureLoaded && m_natureModel.meshCount > 0) {
        for (int i = 0; i < 600; ++i) {
            float dist = RandomFloat(50.0f, kCityRadius * 0.85f);
            float angle = RandomFloat(0.0f, 6.283185f);
            float x = kCityCenter.x + cosf(angle) * dist;
            float z = kCityCenter.y + sinf(angle) * dist;

            if (IsInRunwayZone(x, z)) continue;

            float y = GetHeightAt(x, z);
            if (y < 12.0f) continue;

            int meshIdx = MESH_DECIDUOUS_START + RandomInt(0, MESH_DECIDUOUS_COUNT - 1);
            meshIdx = std::clamp(meshIdx, 0, m_natureModel.meshCount - 1);
            float scale = RandomFloat(0.9f, 1.35f);

            Matrix rotScale = MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(RandomFloat(0.0f, 360.0f) * DEG2RAD));
            Matrix transform = MatrixMultiply(rotScale, MatrixTranslate(x, y, z));
            m_natureInstances.push_back({ &m_natureModel, meshIdx, transform, {x, y, z}, 6.0f * scale });
        }
    }

    // =========================================================================
    // 3. MOUNTAIN ISLAND NATURE (Dense Trees & Bushes, No Logs)
    // =========================================================================
    if (m_natureLoaded && m_natureModel.meshCount > 0) {
        // 3a. Conifer & Pine Trees across slopes
        for (int i = 0; i < 2600; ++i) {
            float dist = RandomFloat(40.0f, kMountainRadius * 0.88f);
            float angle = RandomFloat(0.0f, 6.283185f);
            float x = kMountainCenter.x + cosf(angle) * dist;
            float z = kMountainCenter.y + sinf(angle) * dist;

            float y = GetHeightAt(x, z);
            if (y < 12.0f || y > 750.0f) continue;

            Vector3 norm = GetNormalAt(x, z);
            if (norm.y < 0.48f) continue;

            // Restrict exclusively to Coniferous / Evergreen trees to avoid logs and stumps
            int meshIdx = MESH_CONIFEROUS_START + RandomInt(0, MESH_CONIFEROUS_COUNT - 1);
            meshIdx = std::clamp(meshIdx, 0, m_natureModel.meshCount - 1);

            float scale = RandomFloat(0.95f, 1.65f);
            Matrix rotScale = MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(RandomFloat(0.0f, 360.0f) * DEG2RAD));
            Matrix transform = MatrixMultiply(rotScale, MatrixTranslate(x, y, z));
            m_natureInstances.push_back({ &m_natureModel, meshIdx, transform, {x, y, z}, 7.0f * scale });
        }

        // 3b. Mountain Bushes & Shrubbery
        for (int i = 0; i < 1200; ++i) {
            float dist = RandomFloat(30.0f, kMountainRadius * 0.90f);
            float angle = RandomFloat(0.0f, 6.283185f);
            float x = kMountainCenter.x + cosf(angle) * dist;
            float z = kMountainCenter.y + sinf(angle) * dist;

            float y = GetHeightAt(x, z);
            if (y < 8.0f || y > 680.0f) continue;

            Vector3 norm = GetNormalAt(x, z);
            if (norm.y < 0.50f) continue;

            int meshIdx = MESH_BUSH_START + RandomInt(0, MESH_BUSH_COUNT - 1);
            meshIdx = std::clamp(meshIdx, 0, m_natureModel.meshCount - 1);

            float scale = RandomFloat(0.85f, 1.7f);
            Matrix rotScale = MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(RandomFloat(0.0f, 360.0f) * DEG2RAD));
            Matrix transform = MatrixMultiply(rotScale, MatrixTranslate(x, y, z));
            m_natureInstances.push_back({ &m_natureModel, meshIdx, transform, {x, y, z}, 4.0f * scale });
        }

        // 3c. Mountain Boulders
        for (int i = 0; i < 350; ++i) {
            float dist = RandomFloat(20.0f, kMountainRadius * 0.90f);
            float angle = RandomFloat(0.0f, 6.283185f);
            float x = kMountainCenter.x + cosf(angle) * dist;
            float z = kMountainCenter.y + sinf(angle) * dist;

            float y = GetHeightAt(x, z);
            if (y < 6.0f) continue;

            int meshIdx = MESH_STONE_START + RandomInt(0, MESH_STONE_COUNT - 1);
            meshIdx = std::clamp(meshIdx, 0, m_natureModel.meshCount - 1);
            float scale = RandomFloat(1.0f, 2.5f);

            Matrix rotScale = MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(RandomFloat(0.0f, 360.0f) * DEG2RAD));
            Matrix transform = MatrixMultiply(rotScale, MatrixTranslate(x, y, z));
            m_natureInstances.push_back({ &m_natureModel, meshIdx, transform, {x, y, z}, 4.0f * scale });
        }
    }

    // =========================================================================
    // 4. MEGA ISLAND (NORTH CLIFF) HOUSES & NATURE
    // =========================================================================
    if (m_housesLoaded && m_housesModel.meshCount > 0) {
        Vector2 harborCenter = { kMegaCenter.x - 300.0f, kMegaCenter.y - 1000.0f };
        const float MIN_CLIFF_HOUSE_DIST_SQ = 38.0f * 38.0f; // Spacing to eliminate pileups

        for (int i = 0; i < 200; ++i) {
            // Expanded placement bounds from 500m to 950m
            float dist = RandomFloat(30.0f, 950.0f);
            float angle = RandomFloat(0.0f, 6.283185f);
            float x = harborCenter.x + cosf(angle) * dist;
            float z = harborCenter.y + sinf(angle) * dist;

            float y = GetHeightAt(x, z);
            if (y < 15.0f || y > 110.0f) continue;

            Vector3 norm = GetNormalAt(x, z);
            if (norm.y < 0.88f) continue; // Keep only on flat ground

            // Reject overlapping house coordinates
            bool tooClose = false;
            for (const auto& existing : m_houseInstances) {
                float dx = existing.position.x - x;
                float dz = existing.position.z - z;
                if ((dx * dx + dz * dz) < MIN_CLIFF_HOUSE_DIST_SQ) {
                    tooClose = true;
                    break;
                }
            }
            if (tooClose) continue;

            int houseIdx = RandomInt(0, m_housesModel.meshCount - 1);
            float yaw = RandomFloat(0.0f, 360.0f) * DEG2RAD;

            Matrix rotScale = MatrixMultiply(MatrixScale(3.0f, 3.0f, 3.0f), MatrixRotateY(yaw));
            Matrix transform = MatrixMultiply(rotScale, MatrixTranslate(x, y, z));
            m_houseInstances.push_back({ &m_housesModel, houseIdx, transform, {x, y, z}, 20.0f });
        }
    }

    if (m_natureLoaded && m_natureModel.meshCount > 0) {
        for (int i = 0; i < 2000; ++i) {
            float dist = RandomFloat(50.0f, kMegaRadius * 0.88f);
            float angle = RandomFloat(0.0f, 6.283185f);
            float x = kMegaCenter.x + cosf(angle) * dist;
            float z = kMegaCenter.y + sinf(angle) * dist;

            float y = GetHeightAt(x, z);
            if (y < 12.0f || y > 650.0f) continue;

            Vector3 norm = GetNormalAt(x, z);
            if (norm.y < 0.50f) continue;

            int meshIdx = (y > 220.0f || (z - kMegaCenter.y) > 300.0f)
                ? (MESH_CONIFEROUS_START + RandomInt(0, MESH_CONIFEROUS_COUNT - 1))
                : (MESH_DECIDUOUS_START + RandomInt(0, MESH_DECIDUOUS_COUNT - 1));
            meshIdx = std::clamp(meshIdx, 0, m_natureModel.meshCount - 1);

            float scale = RandomFloat(0.9f, 1.5f);
            Matrix rotScale = MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(RandomFloat(0.0f, 360.0f) * DEG2RAD));
            Matrix transform = MatrixMultiply(rotScale, MatrixTranslate(x, y, z));
            m_natureInstances.push_back({ &m_natureModel, meshIdx, transform, {x, y, z}, 6.0f * scale });
        }

        for (int i = 0; i < 600; ++i) {
            float dist = RandomFloat(30.0f, kMegaRadius * 0.92f);
            float angle = RandomFloat(0.0f, 6.283185f);
            float x = kMegaCenter.x + cosf(angle) * dist;
            float z = kMegaCenter.y + sinf(angle) * dist;

            float y = GetHeightAt(x, z);
            if (y < 8.0f) continue;

            int meshIdx = MESH_STONE_START + RandomInt(0, MESH_STONE_COUNT - 1);
            meshIdx = std::clamp(meshIdx, 0, m_natureModel.meshCount - 1);
            float scale = RandomFloat(1.2f, 3.2f);

            Matrix rotScale = MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(RandomFloat(0.0f, 360.0f) * DEG2RAD));
            Matrix transform = MatrixMultiply(rotScale, MatrixTranslate(x, y, z));
            m_natureInstances.push_back({ &m_natureModel, meshIdx, transform, {x, y, z}, 5.0f * scale });
        }
    }
}

// =========================================================================
// RENDERING & PROPS
// =========================================================================

void TerrainMap::LoadWorld(Engine* engine, const char* mapName) {
    LoadAssetBundles(engine);
    GenerateThreeIslands();
    PopulateThreeIslands();
}

void TerrainMap::Draw(Shader shader) {
    if (!m_terrainsGenerated) return;

    m_cityIslandModel.materials[0].shader = shader;
    DrawModel(m_cityIslandModel, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);

    m_mountainIslandModel.materials[0].shader = shader;
    DrawModel(m_mountainIslandModel, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);

    m_megaIslandModel.materials[0].shader = shader;
    DrawModel(m_megaIslandModel, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);

    m_runwayModel.materials[0].shader = shader;
    DrawModel(m_runwayModel, { kRunwayCenterX, kRunwayElev + 0.12f, kRunwayCenterZ }, 1.0f, WHITE);
}

void TerrainMap::DrawProps(Camera3D camera) {
    const float MAX_HOUSE_DIST_SQ  = 4500.0f * 4500.0f;
    const float MAX_TREE_DIST_SQ   = 6000.0f * 6000.0f;
    const float MAX_DEBRIS_DIST_SQ = 2200.0f * 2200.0f;

    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

    // Houses
    if (m_housesLoaded && !m_houseInstances.empty()) {
        for (const auto& prop : m_houseInstances) {
            Vector3 toProp = Vector3Subtract(prop.position, camera.position);
            float distSq = Vector3LengthSqr(toProp);

            if (distSq > MAX_HOUSE_DIST_SQ) continue;

            if (distSq > 100.0f) {
                float dot = Vector3DotProduct(Vector3Normalize(toProp), camForward);
                if (dot < -0.15f) continue;
            }

            int matIdx = m_housesModel.meshMaterial[prop.meshIndex];
            DrawMesh(m_housesModel.meshes[prop.meshIndex], m_housesModel.materials[matIdx], prop.transform);
        }
    }

    // Nature
    if (m_natureLoaded && !m_natureInstances.empty()) {
        for (const auto& prop : m_natureInstances) {
            Vector3 toProp = Vector3Subtract(prop.position, camera.position);
            float distSq = Vector3LengthSqr(toProp);

            bool isDebris = (prop.meshIndex >= MESH_BUSH_START);
            float cullDistSq = isDebris ? MAX_DEBRIS_DIST_SQ : MAX_TREE_DIST_SQ;

            if (distSq > cullDistSq) continue;

            if (distSq > 100.0f) {
                float dot = Vector3DotProduct(Vector3Normalize(toProp), camForward);
                if (dot < -0.15f) continue;
            }

            int matIdx = m_natureModel.meshMaterial[prop.meshIndex];
            DrawMesh(m_natureModel.meshes[prop.meshIndex], m_natureModel.materials[matIdx], prop.transform);
        }
    }
}

void TerrainMap::Unload() {
    if (m_terrainsGenerated) {
        UnloadModel(m_cityIslandModel);
        UnloadModel(m_mountainIslandModel);
        UnloadModel(m_megaIslandModel);
        UnloadModel(m_runwayModel);
        m_terrainsGenerated = false;
    }
    if (m_housesLoaded) {
        UnloadModel(m_housesModel);
        m_housesLoaded = false;
    }
    if (m_natureLoaded) {
        UnloadModel(m_natureModel);
        m_natureLoaded = false;
    }

    if (m_houseTexture.id != 0) {
        UnloadTexture(m_houseTexture);
        m_houseTexture = { 0 };
    }

    if (m_natureTexture.id != 0) {
        UnloadTexture(m_natureTexture);
        m_natureTexture = { 0 };
    }

    m_houseInstances.clear();
    m_natureInstances.clear();
}