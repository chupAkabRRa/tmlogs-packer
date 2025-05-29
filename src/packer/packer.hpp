#pragma once

#include "hasher/hasher.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Packer {
public:
    Packer(std::unique_ptr<Hasher> hasher);
    void pack(const std::filesystem::path& src_dir, const std::filesystem::path& dst_file);
private:

#pragma pack(push, 1)
    struct FileTableEntry {
        // Array of relative paths sharing the same content
        std::vector<std::string> file_paths;
        uint64_t file_size;
        uint64_t data_offset;
    };

    struct PackHeader {
        // Magic header, just why not :) TMLP = Time Machine Logs Pack
        const char magic[4] = {'T', 'M', 'L', 'P'};
        // Since real table is stored at the end of the pack file, it makes sense
        // to reserve some space at the beginning of the pack file where the offset
        // to file table will be stored
        uint64_t file_table_offset = 0;
    };
#pragma pack(pop)

    using FileTable = std::unordered_map<std::string, FileTableEntry>;
    
    void write_header(std::ofstream& out, const PackHeader& header);
    uint64_t write_file_content(std::ofstream& out, const std::filesystem::path& file_path, uint64_t offset);
    void write_file_table(std::ofstream& out, const FileTable& table);
    FileTable pack_files(std::ofstream& out, const std::filesystem::path& src_dir);

    std::unique_ptr<Hasher> hasher_;
};
