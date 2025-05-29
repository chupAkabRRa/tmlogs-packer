#pragma once

#include "hasher.hpp"

class PicoSha2Hasher : public Hasher {
public:
    std::string compute_hash(const std::filesystem::path& path) override;
};
