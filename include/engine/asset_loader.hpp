#ifndef ASSET_LOADER_HPP
#define ASSET_LOADER_HPP

#include "raylib.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace AssetPacking {

struct AssetInfo {
  std::string name;
  size_t offset; // Offset in the packed file where the asset data begins
  size_t size;   // Size of the asset data
};

class AssetLoader {
public:
  AssetLoader();
  ~AssetLoader();

  Model LoadModelFromAssetData(const std::vector<char> &assetData,
                               const std::string &original_filename);

  bool loadPackedFile(const std::filesystem::path &packedFilePath);
  std::vector<char> getAssetData(const std::string &assetName);
  std::map<std::string, AssetInfo> listAssets() const;
  void UnloadModelAndCleanTempFile(Model model);

  ModelAnimation *LoadAnimationsFromAssetData(const std::vector<char> &data,
                                              int *animCount);

  Texture2D LoadTextureFromAssetData(std::vector<char> &data,
                                     const std::string &extension);

private:
  std::map<std::string, std::ifstream> m_packedFileStreams;
  std::map<std::string, std::map<std::string, AssetInfo>> m_assetMaps;
};

} // namespace AssetPacking

#endif // ASSET_LOADER_HPP
