#include "packer.hpp"
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

Packer::Packer(std::unique_ptr<Hasher> hasher) : hasher_(std::move(hasher)) {}

void Packer::pack(const fs::path& src_dir, const fs::path& dst_pack) {
    // Try to open final packed file for write
    std::ofstream out(dst_pack, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot open packed file for write: " + dst_pack.string());
    }

    // Write basic placeholder header into this file
    PackHeader header;
    write_header(out, header);

    // Pack files from src_dir into the pack file
    FileTable file_table = pack_files(out, src_dir);

    // Write FileTable
    uint64_t file_table_offset = out.tellp();
    write_file_table(out, file_table);

    // Update FileTable offset information
    header.file_table_offset = file_table_offset;
    out.seekp(0);
    write_header(out, header);
}

Packer::FileTable Packer::pack_files(std::ofstream& out, const std::filesystem::path& src_dir) {
    FileTable file_table;
    uint64_t curr_offset = out.tellp();

    // Iterate over files in src_dir and:
    //    * collect info about it into the file table;
    //    * write contents into the final pack file (if needed)
    for (const auto& entry : fs::recursive_directory_iterator(src_dir)) {
        if (!entry.is_regular_file())
        {
            // TODO: This will skip a bunch of stuff like symlinks for example.
            // Should we care?
            continue;
        }

        std::string file_hash = hasher_->compute_hash(entry.path());
        uint64_t file_size = fs::file_size(entry.path());
        std::string rel_path = fs::relative(entry.path(), src_dir).string();

        // Let's try to find file with similar content in our table
        auto it = file_table.find(file_hash);
        if (file_table.find(file_hash) != file_table.end()) {
            // Found? No need to store the data, just extend the array with paths
            it->second.file_paths.push_back(rel_path);
        } else {
            file_table[file_hash] = FileTableEntry{{rel_path}, file_size, curr_offset};
            curr_offset = write_file_content(out, entry.path(), curr_offset);
        }
    }
    return file_table;
}

void Packer::write_header(std::ofstream& out, const PackHeader& header) {
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
}

void Packer::write_file_table(std::ofstream& out, const FileTable& file_table) {
    const char marker[9] = {'F', 'I', 'L', 'E', 'T', 'A', 'B', 'L', 'E'};
    out.write(marker, sizeof(marker));

    uint64_t entry_count = file_table.size();
    out.write(reinterpret_cast<const char*>(&entry_count), sizeof(entry_count));

    for (const auto& [hash, entry] : file_table) {
        uint64_t path_count = entry.file_paths.size();
        out.write(reinterpret_cast<const char*>(&path_count), sizeof(path_count));
        for (const auto& path : entry.file_paths) {
            uint64_t path_len = path.size();
            out.write(reinterpret_cast<const char*>(path_len), sizeof(path_len));
            out.write(path.data(), path_len);
        }
        out.write(reinterpret_cast<const char*>(&entry.file_size), sizeof(entry.file_size));
        out.write(reinterpret_cast<const char*>(&entry.data_offset), sizeof(entry.data_offset));
    }
}

uint64_t Packer::write_file_content(std::ofstream& out, const std::filesystem::path& file_path, uint64_t offset) {
    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
        // What shall we do about it? Should it be recoverable?
        throw std::runtime_error("Failed to open log file for read: " + file_path.string());
    }

    // 4 MB buffer by default
    const std::size_t BufferSize = 4096 * 1024;
    std::vector<char> buffer(BufferSize);

    out.seekp(offset);
    uint64_t next_offset = offset;
    while (in.read(buffer.data(), buffer.size()) || in.gcount()) {
        out.write(buffer.data(), in.gcount());
        next_offset += in.gcount();
    }
    return next_offset;
}