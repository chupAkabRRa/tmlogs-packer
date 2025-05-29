#include <pico_sha2_hasher.hpp>
#include <pico_sha2/picosha2.h>
#include <fstream>
#include <vector>

std::string PicoSha2Hasher::compute_hash(const std::filesystem::path& path) {
    // Since files can be both text and/or binary files - open them as binaries
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open file: " + path.string());
    }

    // 4 MB buffer by default
    const std::size_t BufferSize = 4096 * 1024;
    std::vector<unsigned char> buffer(BufferSize);
    picosha2::hash256_one_by_one hasher;
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        auto bytes_read = file.gcount();
        if (bytes_read > 0) {
            hasher.process(buffer.begin(), buffer.begin() + bytes_read);
        }
    }
    hasher.finish();

    // Hash to string
    std::vector<unsigned char> hash(picosha2::k_digest_size);
    hasher.get_hash_bytes(hash.begin(), hash.end());
    return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
}
