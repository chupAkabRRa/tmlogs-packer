#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "packer.hpp"
#include "hasher/xxhash_hasher.hpp"

namespace fs = std::filesystem;

void write_file(const fs::path& dst, const std::string& content) {
    std::ofstream out(dst, std::ios::binary);
    out << content;
}

bool compare_dirs(const fs::path& dir1, const fs::path& dir2) {
    for (const auto& e : fs::recursive_directory_iterator(dir1)) {
        if (e.is_regular_file()) {
            fs::path rel_path = fs::relative(e.path(), dir1);
            fs::path other_file = dir2 / rel_path;
            if (!fs::exists(other_file) || !fs::is_regular_file(other_file)) {
                std::cout << "Missing file: " << other_file << std::endl;
                return false;
            }
            if (fs::file_size(e.path()) != fs::file_size(other_file)) {
                std::cout << "File size mismathc: " << e.path() << " vs " << other_file << std::endl;
                return false;
            }
            XxHashHasher hasher;
            if (hasher.compute_hash(e.path()) != hasher.compute_hash(other_file)) {
                std::cout << "File contents differ: " << e.path() << std::endl;
                return false;
            }
        }
    }
    return true;
}

int main() {
    fs::path temp_dir = fs::temp_directory_path() / "packer_test";
    fs::path original_dir = temp_dir / "original";
    fs::path packed_file = temp_dir / "archive.pak";
    fs::path unpacked_dir = temp_dir / "unpacked";

    try {
        fs::remove_all(temp_dir);
        fs::create_directories(original_dir);

        fs::create_directories(original_dir / "subdir1" / "nested1");
        fs::create_directories(original_dir / "subdir2");

        write_file(original_dir / "file1.txt", std::string(1024 * 1024 * 20, 'A'));
        write_file(original_dir / "subdir1" / "file2.bin", std::string(1024 * 1024 * 100, '\x42'));
        write_file(original_dir / "subdir1" / "nested1" / "file3.log", std::string(1024 * 1024 * 500, 'B'));
        write_file(original_dir / "subdir2" / "file4.dat", std::string(1024 * 1024 * 128, 'C'));
        write_file(original_dir / "subdir2" / "empty_file.txt", "");
        write_file(original_dir / "subdir1" / "nested1" / "another_empty", "");

        Packer packer(std::make_unique<XxHashHasher>());

        packer.pack(original_dir, packed_file);
        std::cout << "Packing complete\n";

        fs::create_directories(unpacked_dir);
        packer.unpack(packed_file, unpacked_dir);
        std::cout << "Unpaciking complete\n";

        if (compare_dirs(original_dir, unpacked_dir)) {
            std::cout << "Test PASSED!\n";
        } else {
            std::cout << "Test FAILED!\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cout << "Test FAILED with exception: " << e.what() << "\n";
        return 1;
    }
    fs::remove_all(temp_dir);
    std::cout << "Test env cleaned\n";
    return 0;
}
