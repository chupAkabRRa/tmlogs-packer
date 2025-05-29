#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "packer/packer.hpp"
#include "packer/hasher/pico_sha2_hasher.hpp"

namespace fs = std::filesystem;

void help() {
    std::cout << "Usage:\n";
    std::cout << "\tpacker pack <source_directory> <output_file>\n";
    std::cout << "\tpacker unpack <input_file> <target_directory>\n";
    std::cout << "Options:\n";
    std::cout << "\tpack\tPacks the source directory into the specified archive file\n";
    std::cout << "\tunpack\tUnpacks the archive into the target directory\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: command should be provided\n";
        help();
        return 1;
    }

    std::string command = argv[1];

    if (command == "pack") {
        if (argc != 4) {
            std::cerr << "Error: invalid arguments for 'pack' command\n";
            help();
            return 1;
        }

        fs::path src_dir = argv[2];
        fs::path dst_file = argv[3];

        if (!fs::exists(src_dir) || !fs::is_directory(src_dir)) {
            std::cerr << "Error: source directory doesn't exist or is not a directory\n";
            return 1;
        }

        try {
            auto hasher = std::make_unique<PicoSha2Hasher>();
            Packer packer(std::move(hasher));
            packer.pack(src_dir, dst_file);
        } catch (const std::exception& ex) {
            std::cerr << "Packing failed: " << ex.what() << "\n";
            std::cerr << "Feel really sorry for the time traveler :(\n";
            return 1;
        }
    } else {
        std::cerr << "Error: unknow command provided '" << command << "'\n";
        help();
        return 1;
    }

    return 0;
}
