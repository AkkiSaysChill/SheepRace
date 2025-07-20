#ifndef ASSET_LOADER_HPP
#define ASSET_LOADER_HPP

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>

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

    bool loadPackedFile(const std::filesystem::path& packedFilePath);
    std::vector<char> getAssetData(const std::string& assetName);
    std::map<std::string, AssetInfo> listAssets() const;

private:
    std::map<std::string, std::ifstream> m_packedFileStreams;
    std::map<std::string, std::map<std::string, AssetInfo>> m_assetMaps;
};

} // namespace AssetPacking

#endif // ASSET_LOADER_HPP