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

  // load data
  Image image = LoadImageFromMemory(
      extension.c_str(), (const unsigned char *)data.data(), int(data.size()));

  Texture2D texture = LoadTextureFromImage(image);

  UnloadImage(image);

  return texture;
}

// get animation data
ModelAnimation *
AssetLoader::LoadAnimationsFromAssetData(const std::vector<char> &data,
                                         int *animCount) {
  if (data.empty()) {
    *animCount = 0;
    return nullptr;
  }

  // 1. Write memory data to a temporary file on disk
  const char *tempFileName = "temp_anim_load.glb";
  std::ofstream tempFile(tempFileName, std::ios::binary);
  tempFile.write(data.data(), data.size());
  tempFile.close();

  // 2. Let Raylib load from that file
  ModelAnimation *anims = LoadModelAnimations(tempFileName, animCount);

  // 3. Delete the temporary file
  std::remove(tempFileName);

  return anims;
}

Model AssetLoader::LoadModelFromAssetData(
    const std::vector<char> &asset_data, const std::string &original_filename) {
  if (asset_data.empty()) {
    return {0}; // Return empty model
  }

  // Create a temporary file
  std::string temp_filename = fs::temp_directory_path().string() + "/" +
                              fs::path(original_filename).filename().string();
  std::ofstream temp_ofs(temp_filename, std::ios::binary);
  if (!temp_ofs.is_open()) {
    std::cerr << "Error: Could not create temporary file for asset: "
              << original_filename << std::endl;
    return {0};
  }
  temp_ofs.write(reinterpret_cast<const char *>(asset_data.data()),
                 asset_data.size());
  temp_ofs.close();

  // Load model from temporary file
  Model model = LoadModel(temp_filename.c_str());

  // Delete temporary file
  fs::remove(temp_filename);

  return model;
}

bool AssetLoader::loadPackedFile(const std::filesystem::path &packedFilePath) {
  std::string path_str = packedFilePath.string();
  if (m_packedFileStreams.count(path_str))
    return true;

  m_packedFileStreams[path_str].open(packedFilePath, std::ios::binary);
  if (!m_packedFileStreams[path_str].is_open())
    return false;

  // Read number of files (Fixed 64-bit int)
  uint64_t num_files;
  m_packedFileStreams[path_str].read(reinterpret_cast<char *>(&num_files),
                                     sizeof(uint64_t));

  m_assetMaps[path_str].clear();

  // Loop strictly through the TOC (Header)
  for (size_t i = 0; i < num_files; ++i) {
    // Read Name Length
    uint64_t name_len;
    m_packedFileStreams[path_str].read(reinterpret_cast<char *>(&name_len),
                                       sizeof(uint64_t));

    // Read Name
    std::string name(name_len, '\0');
    m_packedFileStreams[path_str].read(&name[0], name_len);

    // Read Size and Offset (We now know exactly where data is!)
    uint64_t file_size, file_offset;
    m_packedFileStreams[path_str].read(reinterpret_cast<char *>(&file_size),
                                       sizeof(uint64_t));
    m_packedFileStreams[path_str].read(reinterpret_cast<char *>(&file_offset),
                                       sizeof(uint64_t));

    AssetInfo info;
    info.name = name;
    info.size = (size_t)file_size; // Cast back to size_t for internal use
    info.offset = (size_t)file_offset;

    m_assetMaps[path_str][name] = info;
  }

  // No need to seek/skip data anymore. We are done!
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
