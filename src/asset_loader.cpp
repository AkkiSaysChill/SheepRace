#include "asset_loader.hpp"
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

Model AssetLoader::LoadModelFromAssetData(const std::vector<char> &asset_data,
                                           const std::string &original_filename) {
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
  if (m_packedFileStreams.count(path_str)) {
    std::cerr << "Warning: Packed file " << packedFilePath << " already loaded."
              << std::endl;
    return true;
  }

  m_packedFileStreams[path_str].open(packedFilePath, std::ios::binary);
  if (!m_packedFileStreams[path_str].is_open()) {
    std::cerr << "Error: Could not open packed file " << packedFilePath
              << std::endl;
    m_packedFileStreams.erase(path_str);
    return false;
  }

  size_t num_files;
  m_packedFileStreams[path_str].read(reinterpret_cast<char *>(&num_files),
                                     sizeof(num_files));

  m_assetMaps[path_str].clear();
  for (size_t i = 0; i < num_files; ++i) {
    size_t name_len;
    m_packedFileStreams[path_str].read(reinterpret_cast<char *>(&name_len),
                                       sizeof(name_len));
    std::string name(name_len, '\0');
    m_packedFileStreams[path_str].read(&name[0], name_len);

    size_t file_size;
    m_packedFileStreams[path_str].read(reinterpret_cast<char *>(&file_size),
                                       sizeof(file_size));

    AssetInfo info;
    info.name = name;
    info.offset = m_packedFileStreams[path_str].tellg();
    info.size = file_size;
    m_assetMaps[path_str][name] = info;

    m_packedFileStreams[path_str].seekg(file_size, std::ios::cur);
  }
  return true;
}

std::vector<char> AssetLoader::getAssetData(const std::string &assetName) {
  for (auto const &[filePath, assetMap] : m_assetMaps) {
    auto it = assetMap.find(assetName);
    if (it != assetMap.end()) {
      const AssetInfo &info = it->second;
      std::vector<char> data(info.size);

      m_packedFileStreams[filePath].seekg(info.offset);
      m_packedFileStreams[filePath].read(data.data(), info.size);

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
