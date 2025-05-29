#pragma once

#include <string>
#include <filesystem>

class Hasher {
public:
    virtual ~Hasher() = default;
    virtual std::string compute_hash(const std::filesystem::path& file_path) = 0;
};
