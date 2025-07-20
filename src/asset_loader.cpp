#include "asset_loader.hpp"
#include <iostream>

namespace AssetPacking {

AssetLoader::AssetLoader() {
}

AssetLoader::~AssetLoader() {
    for (auto& pair : m_packedFileStreams) {
        if (pair.second.is_open()) {
            pair.second.close();
        }
    }
}

bool AssetLoader::loadPackedFile(const std::filesystem::path& packedFilePath) {
    std::string path_str = packedFilePath.string();
    if (m_packedFileStreams.count(path_str)) {
        std::cerr << "Warning: Packed file " << packedFilePath << " already loaded." << std::endl;
        return true;
    }

    m_packedFileStreams[path_str].open(packedFilePath, std::ios::binary);
    if (!m_packedFileStreams[path_str].is_open()) {
        std::cerr << "Error: Could not open packed file " << packedFilePath << std::endl;
        m_packedFileStreams.erase(path_str);
        return false;
    }

    size_t num_files;
    m_packedFileStreams[path_str].read(reinterpret_cast<char*>(&num_files), sizeof(num_files));

    m_assetMaps[path_str].clear();
    for (size_t i = 0; i < num_files; ++i) {
        size_t name_len;
        m_packedFileStreams[path_str].read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        std::string name(name_len, '\0');
        m_packedFileStreams[path_str].read(&name[0], name_len);

        size_t file_size;
        m_packedFileStreams[path_str].read(reinterpret_cast<char*>(&file_size), sizeof(file_size));

        AssetInfo info;
        info.name = name;
        info.offset = m_packedFileStreams[path_str].tellg();
        info.size = file_size;
        m_assetMaps[path_str][name] = info;

        m_packedFileStreams[path_str].seekg(file_size, std::ios::cur);
    }
    return true;
}

std::vector<char> AssetLoader::getAssetData(const std::string& assetName) {
    for (auto const& [filePath, assetMap] : m_assetMaps) {
        auto it = assetMap.find(assetName);
        if (it != assetMap.end()) {
            const AssetInfo& info = it->second;
            std::vector<char> data(info.size);

            m_packedFileStreams[filePath].seekg(info.offset);
            m_packedFileStreams[filePath].read(data.data(), info.size);

            return data;
        }
    }
    std::cerr << "Error: Asset " << assetName << " not found in any loaded packed file." << std::endl;
    return {};
}

std::map<std::string, AssetInfo> AssetLoader::listAssets() const {
    std::map<std::string, AssetInfo> allAssets;
    for (auto const& [filePath, assetMap] : m_assetMaps) {
        allAssets.insert(assetMap.begin(), assetMap.end());
    }
    return allAssets;
}

} // namespace AssetPacking