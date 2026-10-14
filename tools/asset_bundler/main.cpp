#include <algorithm> 
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint> 

namespace fs = std::filesystem;

// Helper struct for the TOC
struct FileEntry {
    std::string relative_path;
    uint64_t size;
    uint64_t offset;
};

void pack_directory(const fs::path& source_dir, const fs::path& output_file) {
    std::ofstream ofs(output_file, std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "Error: Could not open output file." << std::endl;
        return;
    }

    std::vector<FileEntry> entries;
    uint64_t current_offset = 0;

    // gather metadata
    for (const auto& entry : fs::recursive_directory_iterator(source_dir)) {
        if (fs::is_regular_file(entry.path())) {
            // Skip any file we can't actually open for reading. This check
            // has to happen BEFORE the file is added to the TOC: previously
            // an unopenable file was still given a size/offset in the
            // header, but its bytes were silently skipped during the data
            // write pass below. That shifted every subsequent file's real
            // data earlier than its recorded offset, corrupting the whole
            // rest of the archive - which is why assets loaded out of it
            // (e.g. audio) could come back truncated/garbled and crash the
            // decoder in-game.
            std::ifstream probe(entry.path(), std::ios::binary);
            if (!probe.is_open()) {
                std::cerr << "Warning: could not open '" << entry.path()
                          << "', skipping it (archive would otherwise be corrupted)."
                          << std::endl;
                continue;
            }
            probe.close();

            FileEntry meta;
            // Normalize path
            meta.relative_path = fs::relative(entry.path(), source_dir).string();
            std::replace(meta.relative_path.begin(), meta.relative_path.end(), '\\', '/');

            meta.size = fs::file_size(entry.path());
            entries.push_back(meta);
        }
    }

    // calc header size
    // Header structure:
    // [TotalFiles (8 bytes)]
    // For each file: [NameLen (8 bytes)] + [NameString (N bytes)] + [Size (8 bytes)] + [Offset (8 bytes)]

    uint64_t header_size = sizeof(uint64_t);
    for (const auto& entry : entries) {
        header_size += sizeof(uint64_t) + entry.relative_path.length();
        header_size += sizeof(uint64_t); // Size
        header_size += sizeof(uint64_t); // Offset
    }

    // assign offset
    // Data starts immediately after the header
    current_offset = header_size;
    for (auto& entry : entries) {
        entry.offset = current_offset;
        current_offset += entry.size;
    }

    // write header (TOC)
    uint64_t num_files = entries.size();
    ofs.write(reinterpret_cast<const char*>(&num_files), sizeof(num_files));

    for (const auto& entry : entries) {
        uint64_t name_len = entry.relative_path.length();
        ofs.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        ofs.write(entry.relative_path.c_str(), name_len);

        // Use fixed types for portability!
        ofs.write(reinterpret_cast<const char*>(&entry.size), sizeof(uint64_t));
        ofs.write(reinterpret_cast<const char*>(&entry.offset), sizeof(uint64_t));
    }

    // write data chunks
    char buffer[4096]; // Use small buffer to save RAM
    for (const auto& entry : entries) {
        fs::path full_path = source_dir / entry.relative_path;
        std::ifstream ifs(full_path, std::ios::binary);

        // By this point the header already reserved exactly entry.size
        // bytes at entry.offset for this file. Every earlier file passed
        // the same open check during the metadata pass, so a failure here
        // means the file vanished/became unreadable mid-run - silently
        // skipping it (as before) would leave a hole and desync every
        // offset after it, corrupting the archive. Treat it as fatal.
        if (!ifs.is_open()) {
            std::cerr << "Error: '" << full_path
                      << "' became unreadable while packing; aborting to avoid "
                         "producing a corrupt archive." << std::endl;
            ofs.close();
            fs::remove(output_file);
            return;
        }

        uint64_t bytes_written = 0;
        while (ifs.read(buffer, sizeof(buffer))) {
            ofs.write(buffer, ifs.gcount());
            bytes_written += (uint64_t)ifs.gcount();
        }
        if (ifs.gcount() > 0) {
            ofs.write(buffer, ifs.gcount());
            bytes_written += (uint64_t)ifs.gcount();
        }

        if (bytes_written != entry.size) {
            // File shrank/grew between the metadata pass and now - same
            // corruption risk as above.
            std::cerr << "Error: '" << full_path
                      << "' changed size while packing; aborting to avoid "
                         "producing a corrupt archive." << std::endl;
            ofs.close();
            fs::remove(output_file);
            return;
        }
    }

    if (!ofs) {
        std::cerr << "Error: a write to '" << output_file
                  << "' failed (disk full?); archive is corrupt." << std::endl;
        ofs.close();
        fs::remove(output_file);
        return;
    }

    ofs.close();
    std::cout << "Packed " << num_files << " files." << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <source_directory> <output_file>"
              << std::endl;
    return 1;
  }

  fs::path source_dir = argv[1];
  fs::path output_file = argv[2];

  if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
    std::cerr << "Error: Source directory " << source_dir
              << " does not exist or is not a directory." << std::endl;
    return 1;
  }

  pack_directory(source_dir, output_file);

  return 0;
}
