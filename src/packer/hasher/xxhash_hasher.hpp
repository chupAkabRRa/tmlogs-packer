#pragma once

#include "hasher.hpp"

class XxHashHasher : public Hasher {
public:
    std::string compute_hash(const std::filesystem::path& path) override;
};
