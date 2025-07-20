#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm> // Required for std::replace

namespace fs = std::filesystem;

void pack_directory(const fs::path& source_dir, const fs::path& output_file) {
    std::ofstream ofs(output_file, std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "Error: Could not open output file " << output_file << std::endl;
        return;
    }

    std::vector<fs::path> files_to_pack;
    for (const auto& entry : fs::recursive_directory_iterator(source_dir)) {
        if (fs::is_regular_file(entry.path())) {
            files_to_pack.push_back(entry.path());
        }
    }

    // Write the number of files
    size_t num_files = files_to_pack.size();
    ofs.write(reinterpret_cast<const char*>(&num_files), sizeof(num_files));

    for (const auto& file_path : files_to_pack) {
        // Get relative path
        std::string relative_path = fs::relative(file_path, source_dir).string();
        std::replace(relative_path.begin(), relative_path.end(), '\\', '/'); // Normalize path separators

        // Write file name length and name
        size_t name_len = relative_path.length();
        ofs.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        ofs.write(relative_path.c_str(), name_len);

        // Write file size and data
        std::ifstream ifs(file_path, std::ios::binary);
        if (!ifs.is_open()) {
            std::cerr << "Warning: Could not open input file " << file_path << ". Skipping." << std::endl;
            continue;
        }
        size_t file_size = fs::file_size(file_path);
        ofs.write(reinterpret_cast<const char*>(&file_size), sizeof(file_size));

        std::vector<char> buffer(file_size);
        ifs.read(buffer.data(), file_size);
        ofs.write(buffer.data(), file_size);
        ifs.close();
    }

    ofs.close();
    std::cout << "Successfully packed " << num_files << " files into " << output_file << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <source_directory> <output_file>" << std::endl;
        return 1;
    }

    fs::path source_dir = argv[1];
    fs::path output_file = argv[2];

    if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
        std::cerr << "Error: Source directory " << source_dir << " does not exist or is not a directory." << std::endl;
        return 1;
    }

    pack_directory(source_dir, output_file);

    return 0;
}