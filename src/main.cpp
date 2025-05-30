#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "packer/hasher/pico_sha2_hasher.hpp"
#include "packer/hasher/xxhash_hasher.hpp"
#include "packer/packer.hpp"
#include "utils/logger.hpp"

namespace fs = std::filesystem;

void help() {
    std::cout << "Usage:\n";
    std::cout << "\tpacker [--log-level=<level>] pack <source_directory> <output_file>\n";
    std::cout << "\tpacker [--log-level=<level>] unpack <input_file> <target_directory>\n";
    std::cout << "Options:\n";
    std::cout << "\tpack\tPacks the source directory into the specified archive file\n";
    std::cout << "\tunpack\tUnpacks the archive into the target directory\n";
    std::cout << "\t--log-level\tLogging level: error, warning, info, none (default: info)";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: command should be provided\n";
        help();
        return 1;
    }

    std::string command;
    std::vector<std::string> args;

    // Parse command line args
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.starts_with("--log-level=")) {
            auto level_pos = arg.find_first_of('=') + 1;
            if (arg.size() < level_pos) {
                std::cerr << "Error: log level must be specified after --log-level switch\n";
                return 1;
            }
            auto log_level = Logger::level_from_string(arg.substr(level_pos));
            Logger::set_min_log_level(log_level);
        } else if (command.empty()) {
            command = arg;
        } else {
            args.push_back(arg);
        }
    }

    if (command == "pack") {
        if (args.size() != 2) {
            std::cerr << "Error: invalid arguments for 'pack' command\n";
            help();
            return 1;
        }

        fs::path src_dir = args[0];
        fs::path dst_file = args[1];

        if (!fs::exists(src_dir) || !fs::is_directory(src_dir)) {
            std::cerr << "Error: source directory doesn't exist or is not a directory\n";
            return 1;
        }

        try {
            auto hasher = std::make_unique<XxHashHasher>();
            Packer packer(std::move(hasher));
            packer.pack(src_dir, dst_file);
        } catch (const std::exception& ex) {
            std::cerr << "Packing failed: " << ex.what() << "\n";
            std::cerr << "Feel really sorry for the time traveller :(\n";
            return 1;
        }
    } else if (command == "unpack") {
        if (args.size() != 2) {
            std::cerr << "Error: invalid arguments for 'unpack' command\n";
            help();
            return 1;
        }

        fs::path pack_file = args[0];
        fs::path dst_dir = args[1];

        if (!fs::exists(pack_file) || !fs::is_regular_file(pack_file)) {
            std::cerr << "Error: pack file doesn't exist or is not a file\n";
            return 1;
        }

        if (!fs::exists(dst_dir)) {
            std::cout << "Warning: target directory doesn't exist. Creating " << dst_dir.string() << "\n";
            fs::create_directories(dst_dir);
        } else if (!fs::is_directory(dst_dir)) {
            std::cerr << "Error: target path exists but is not a directory\n";
            return 1;
        }

        try {
            Packer packer;
            packer.unpack(pack_file, dst_dir);
        } catch (const std::exception& ex) {
            std::cerr << "Unpacking failed: " << ex.what() << "\n";
            std::cerr << "Feel really sorry for the time traveller :(\n";
            return 1;
        }
    } else {
        std::cerr << "Error: unknow command provided '" << command << "'\n";
        help();
        return 1;
    }

    return 0;
}
