#include "packer.hpp"

#include <fstream>
#include <iostream>

#include "utils/logger.hpp"

namespace fs = std::filesystem;

Packer::Packer(std::unique_ptr<Hasher> hasher) : hasher_(std::move(hasher)) {}

void Packer::pack(const fs::path& src_dir, const fs::path& pack_file) {
    if (!hasher_) {
        throw std::runtime_error("Cannot pack files without hasher provided\n");
    }

    // Try to open final packed file for write
    std::ofstream out(pack_file, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot open packed file for write: " + pack_file.string());
    }

    Logger(LogLevel::INFO) << "Packing log files from " << src_dir.string()
                           << " into: " << pack_file.string();

    // Write basic placeholder header into this file
    PackHeader header;
    write_header(out, header);

    // Pack files from src_dir into the pack file
    FileTable file_table = pack_files(out, src_dir);

    Logger(LogLevel::INFO) << "SUMMARY: " << file_table.size() << " files packed";

    // Write FileTable
    uint64_t file_table_offset = out.tellp();
    write_file_table(out, file_table);

    // Update FileTable offset information
    header.file_table_offset = file_table_offset;
    out.seekp(0);
    write_header(out, header);
}

void Packer::unpack(const fs::path& pack_file, const std::filesystem::path& dst_dir) {
    std::ifstream in(pack_file, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open pack file for read: " + pack_file.string());
    }

    Logger(LogLevel::INFO) << "Unpacking files from " << pack_file.string()
                           << " into " << dst_dir.string();

    PackHeader header = read_header(in);
    auto file_entries = read_file_table(in, header.file_table_offset);

    for (const auto& entry : file_entries) {
        unpack_file_content(in, entry, dst_dir);
    }

    Logger(LogLevel::INFO) << "SUMMARY: " << file_entries.size() << " files unpacked";
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

        Logger(LogLevel::INFO) << "Packing file " << entry.path().filename();

        // Hash file's content to determine its uniqueness
        Logger(LogLevel::INFO) << "\tHashing first...";
        std::string file_hash = hasher_->compute_hash(entry.path());
        Logger(LogLevel::INFO) << "\tHashing complete!";
        // Collect other file data
        uint64_t file_size = fs::file_size(entry.path());
        std::string rel_path = fs::relative(entry.path(), src_dir).string();

        // Let's try to find file with similar content in our table
        auto it = file_table.find(file_hash);
        if (file_table.find(file_hash) != file_table.end()) {
            // Found? No need to store the data, just extend the array with paths
            it->second.file_paths.push_back(rel_path);
            Logger(LogLevel::INFO) << "\tFile with similar content discovered. No need to pack";
        } else {
            file_table[file_hash] = FileTableEntry{{rel_path}, file_size, curr_offset};
            Logger(LogLevel::INFO) << "\tMoving to pack file...";
            curr_offset = write_file_content(out, entry.path(), curr_offset);
            Logger(LogLevel::INFO) << "\tMoving complete!";
        }

        Logger(LogLevel::INFO) << "Packing complete!";
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
            out.write(reinterpret_cast<const char*>(&path_len), sizeof(path_len));
            out.write(path.data(), path_len);
        }
        out.write(reinterpret_cast<const char*>(&entry.file_size), sizeof(entry.file_size));
        out.write(reinterpret_cast<const char*>(&entry.data_offset), sizeof(entry.data_offset));
    }
}

uint64_t Packer::write_file_content(std::ofstream& out, const std::filesystem::path& file_path, uint64_t offset_in_pack) {
    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
        // What shall we do about it? Should it be recoverable?
        throw std::runtime_error("Failed to open log file for read: " + file_path.string());
    }

    // 4 MB buffer by default
    const std::size_t BufferSize = 4096 * 1024;
    std::vector<char> buffer(BufferSize);

    out.seekp(offset_in_pack);
    uint64_t next_offset = offset_in_pack;
    while (in.read(buffer.data(), buffer.size()) || in.gcount()) {
        out.write(buffer.data(), in.gcount());
        next_offset += in.gcount();
    }
    return next_offset;
}

Packer::PackHeader Packer::read_header(std::ifstream& in) {
    PackHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (std::string(header.magic, sizeof(header.magic)) != "TMLP") {
        throw std::runtime_error("Invalid pack format: missing magic sequence");
    }
    return header;
}

std::vector<Packer::FileTableEntry> Packer::read_file_table(std::ifstream& in, uint64_t file_table_offset) {
    in.seekg(file_table_offset);

    char marker[9];
    in.read(marker, sizeof(marker));
    if (std::string(marker, sizeof(marker)) != "FILETABLE") {
        throw std::runtime_error("Invalid pack format: missing file table marker");
    }

    uint64_t entry_count;
    in.read(reinterpret_cast<char*>(&entry_count), sizeof(entry_count));

    std::vector<FileTableEntry> file_entries;
    for (uint64_t i = 0; i < entry_count; i++) {
        uint64_t path_count;
        in.read(reinterpret_cast<char*>(&path_count), sizeof(path_count));

        std::vector<std::string> paths;
        for (uint64_t j = 0; j < path_count; j++) {
            uint64_t path_len;
            in.read(reinterpret_cast<char*>(&path_len), sizeof(path_len));
            std::string path(path_len, '\0');
            in.read(&path[0], path_len);
            paths.push_back(path);
        }

        uint64_t size;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));
        uint64_t offset;
        in.read(reinterpret_cast<char*>(&offset), sizeof(offset));

        file_entries.emplace_back(FileTableEntry{paths, size, offset});
    }
    return file_entries;
}

void Packer::unpack_file_content(std::ifstream& in, const FileTableEntry& entry, const fs::path& dst_dir) {
    for (const auto& relative_path : entry.file_paths) {
        fs::path full_path = dst_dir / relative_path;
        fs::create_directories(full_path.parent_path());
        std::ofstream out(full_path, std::ios::binary);
        if (!out) {
            // What shall we do about it? Should it be recoverable?
            throw std::runtime_error("Failed to create output file: " + full_path.string());
        }

        in.seekg(entry.data_offset);

        Logger(LogLevel::INFO) << "Unpacking " << full_path.string();

         // 4 MB buffer by default
        const std::size_t BufferSize = 4096 * 1024;
        std::vector<char> buffer(BufferSize);
        uint64_t remaining = entry.file_size;
        while (remaining > 0) {
            uint64_t to_read = std::min<uint64_t>(buffer.size(), remaining);
            in.read(buffer.data(), to_read);
            out.write(buffer.data(), to_read);
            remaining -= to_read;
        }

        Logger(LogLevel::INFO) << "Unpacking complete!";
    }
}
