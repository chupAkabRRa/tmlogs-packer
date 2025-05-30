#include "xxhash_hasher.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

#include "xxhash/xxhash.h"

std::string hash_to_hex(uint64_t hash) {
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return oss.str();
}

std::string XxHashHasher::compute_hash(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open file: " + path.string());
    }

    XXH64_state_t* state = XXH64_createState();
    XXH64_reset(state, 0);

    // 4 MB buffer by default
    const std::size_t BufferSize = 4096 * 1024;
    std::vector<unsigned char> buffer(BufferSize);
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        auto bytes_read = file.gcount();
        if (bytes_read > 0) {
            XXH64_update(state, buffer.data(), bytes_read);
        }
    }

    uint64_t hash = XXH64_digest(state);
    XXH64_freeState(state);

    return hash_to_hex(hash);
}
