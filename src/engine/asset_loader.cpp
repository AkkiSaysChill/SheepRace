#include "engine/asset_loader.hpp"
#include "raylib.h"
#include <cstdio>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace AssetPacking {

AssetLoader::AssetLoader() {}

AssetLoader::~AssetLoader() {
  for (auto &pair : m_packedFileStreams) {
    if (pair.second.is_open()) {
      pair.second.close();
    }
  }
}

Texture2D AssetLoader::LoadTextureFromAssetData(std::vector<char> &data,
                                                const std::string &extension) {
  if (data.empty())
    return {0};

  Image image = LoadImageFromMemory(
      extension.c_str(), 
      reinterpret_cast<const unsigned char *>(data.data()), 
      static_cast<int>(data.size()));

  Texture2D texture = LoadTextureFromImage(image);
  UnloadImage(image);

  return texture;
}

ModelAnimation *
AssetLoader::LoadAnimationsFromAssetData(const std::vector<char> &data,
                                         int *animCount) {
  if (data.empty()) {
    *animCount = 0;
    return nullptr;
  }

  // Write memory buffer to isolated local runtime cache file
  const char *tempAnimFile = "temp_load_anim.glb";
  SaveFileData(tempAnimFile, const_cast<char *>(data.data()), static_cast<int>(data.size()));

  ModelAnimation *anims = LoadModelAnimations(tempAnimFile, animCount);

  // Clean up cache file immediately
  std::remove(tempAnimFile);

  return anims;
}

Model AssetLoader::LoadModelFromAssetData(
    const std::vector<char> &asset_data, const std::string &original_filename) {
  if (asset_data.empty()) {
    return {0};
  }

  // Write model buffer to deterministic cache path using Raylib's native file writer
  std::string tempModelPath = "temp_load_" + fs::path(original_filename).filename().string();
  SaveFileData(tempModelPath.c_str(), const_cast<char *>(asset_data.data()), static_cast<int>(asset_data.size()));

  // Raylib loads the model geometry, textures, and materials from the temporary file
  Model model = LoadModel(tempModelPath.c_str());

  // Clean up local temp file immediately (Model data is already copied into GPU/RAM)
  std::remove(tempModelPath.c_str());

  return model;
}

bool AssetLoader::loadPackedFile(const std::filesystem::path &packedFilePath) {
  std::string path_str = packedFilePath.string();
  if (m_packedFileStreams.count(path_str))
    return true;

  m_packedFileStreams[path_str].open(packedFilePath, std::ios::binary);
  if (!m_packedFileStreams[path_str].is_open())
    return false;

  uint64_t num_files = 0;
  m_packedFileStreams[path_str].read(reinterpret_cast<char *>(&num_files),
                                     sizeof(uint64_t));

  m_assetMaps[path_str].clear();

  for (size_t i = 0; i < num_files; ++i) {
    uint64_t name_len = 0;
    m_packedFileStreams[path_str].read(reinterpret_cast<char *>(&name_len),
                                       sizeof(uint64_t));

    std::string name(name_len, '\0');
    m_packedFileStreams[path_str].read(&name[0], name_len);

    uint64_t file_size = 0, file_offset = 0;
    m_packedFileStreams[path_str].read(reinterpret_cast<char *>(&file_size),
                                       sizeof(uint64_t));
    m_packedFileStreams[path_str].read(reinterpret_cast<char *>(&file_offset),
                                       sizeof(uint64_t));

    AssetInfo info;
    info.name = name;
    info.size = static_cast<size_t>(file_size);
    info.offset = static_cast<size_t>(file_offset);

    m_assetMaps[path_str][name] = info;
  }

  return true;
}

std::vector<char> AssetLoader::getAssetData(const std::string &assetName) {
  for (auto const &[filePath, assetMap] : m_assetMaps) {
    auto it = assetMap.find(assetName);
    if (it != assetMap.end()) {
      const AssetInfo &info = it->second;
      std::vector<char> data(info.size + 1);

      m_packedFileStreams[filePath].seekg(info.offset);
      m_packedFileStreams[filePath].read(data.data(), info.size);

      data[info.size] = '\0';

      return data;
    }
  }
  std::cerr << "Error: Asset " << assetName
            << " not found in any loaded packed file." << std::endl;
  return {};
}

std::map<std::string, AssetInfo> AssetLoader::listAssets() const {
  std::map<std::string, AssetInfo> allAssets;
  for (auto const &[filePath, assetMap] : m_assetMaps) {
    allAssets.insert(assetMap.begin(), assetMap.end());
  }
  return allAssets;
}

} // namespace AssetPacking