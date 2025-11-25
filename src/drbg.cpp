/**
 * @file drbg.cpp
 * @brief Implementation of DRBG algorithms
 */

#include "drbg.hpp"
#include <stdexcept>
#include <algorithm>

// ============================================================================
// SHA-256 Constants and Implementation
// ============================================================================

namespace {
    // SHA-256 constants
    constexpr uint32_t SHA256_K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    inline uint32_t rotr(uint32_t x, int n) {
        return (x >> n) | (x << (32 - n));
    }

    inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (~x & z);
    }

    inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (x & z) ^ (y & z);
    }

    inline uint32_t sigma0(uint32_t x) {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }

    inline uint32_t sigma1(uint32_t x) {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }

    inline uint32_t gamma0(uint32_t x) {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }

    inline uint32_t gamma1(uint32_t x) {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }
}

std::array<uint8_t, 32> Hash_DRBG::sha256(const std::vector<uint8_t>& data) {
    // Initial hash values
    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // Pre-processing: adding padding bits
    std::vector<uint8_t> padded = data;
    size_t original_len = data.size();
    size_t original_bit_len = original_len * 8;
    
    padded.push_back(0x80);
    while ((padded.size() % 64) != 56) {
        padded.push_back(0x00);
    }
    
    // Append length in bits as 64-bit big-endian
    for (int i = 7; i >= 0; --i) {
        padded.push_back(static_cast<uint8_t>((original_bit_len >> (i * 8)) & 0xFF));
    }

    // Process each 512-bit block
    for (size_t chunk = 0; chunk < padded.size(); chunk += 64) {
        uint32_t w[64];
        
        // Copy chunk into first 16 words
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(padded[chunk + i * 4]) << 24) |
                   (static_cast<uint32_t>(padded[chunk + i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(padded[chunk + i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(padded[chunk + i * 4 + 3]));
        }
        
        // Extend to 64 words
        for (int i = 16; i < 64; ++i) {
            w[i] = gamma1(w[i-2]) + w[i-7] + gamma0(w[i-15]) + w[i-16];
        }

        // Initialize working variables
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];

        // Compression function
        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = hh + sigma1(e) + ch(e, f, g) + SHA256_K[i] + w[i];
            uint32_t t2 = sigma0(a) + maj(a, b, c);
            hh = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        // Add to hash
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    // Produce final hash
    std::array<uint8_t, 32> result;
    for (int i = 0; i < 8; ++i) {
        result[i * 4] = static_cast<uint8_t>((h[i] >> 24) & 0xFF);
        result[i * 4 + 1] = static_cast<uint8_t>((h[i] >> 16) & 0xFF);
        result[i * 4 + 2] = static_cast<uint8_t>((h[i] >> 8) & 0xFF);
        result[i * 4 + 3] = static_cast<uint8_t>(h[i] & 0xFF);
    }
    
    return result;
}

std::array<uint8_t, 32> HMAC_DRBG::sha256(const std::vector<uint8_t>& data) {
    return Hash_DRBG::sha256(data);
}

// ============================================================================
// CTR-DRBG Implementation
// ============================================================================

CTR_DRBG::CTR_DRBG(const std::vector<uint8_t>& seed) {
    key.fill(0);
    counter.fill(0);
    reseed_counter = 1;
    
    // Initial update with seed
    update(seed);
}

std::array<uint8_t, CTR_DRBG::BLOCK_SIZE> CTR_DRBG::encrypt_block(
    const std::array<uint8_t, BLOCK_SIZE>& block) {
    
    std::array<uint8_t, BLOCK_SIZE> state = block;
    
    // Simple SPN cipher: 10 rounds
    for (int round = 0; round < 10; ++round) {
        // Add round key (derived from main key)
        for (size_t i = 0; i < BLOCK_SIZE; ++i) {
            state[i] ^= key[(round * BLOCK_SIZE + i) % KEY_SIZE];
        }
        
        // SubBytes (S-box substitution)
        for (size_t i = 0; i < BLOCK_SIZE; ++i) {
            state[i] = SBOX[state[i]];
        }
        
        // ShiftRows (simplified permutation)
        std::array<uint8_t, BLOCK_SIZE> temp = state;
        for (size_t i = 0; i < BLOCK_SIZE; ++i) {
            state[i] = temp[(i + (i / 4)) % BLOCK_SIZE];
        }
        
        // MixColumns (simplified linear transformation)
        if (round < 9) {  // Skip in last round
            for (size_t i = 0; i < BLOCK_SIZE; i += 4) {
                uint8_t t = state[i] ^ state[i+1] ^ state[i+2] ^ state[i+3];
                uint8_t u = state[i];
                state[i] ^= t ^ ((state[i] ^ state[i+1]) << 1);
                state[i+1] ^= t ^ ((state[i+1] ^ state[i+2]) << 1);
                state[i+2] ^= t ^ ((state[i+2] ^ state[i+3]) << 1);
                state[i+3] ^= t ^ ((state[i+3] ^ u) << 1);
            }
        }
    }
    
    return state;
}

void CTR_DRBG::increment_counter() {
    for (int i = BLOCK_SIZE - 1; i >= 0; --i) {
        if (++counter[i] != 0) break;
    }
}

void CTR_DRBG::update(const std::vector<uint8_t>& provided_data) {
    std::vector<uint8_t> temp;
    
    // Generate enough blocks to fill key + counter
    while (temp.size() < KEY_SIZE + BLOCK_SIZE) {
        increment_counter();
        auto block = encrypt_block(counter);
        temp.insert(temp.end(), block.begin(), block.end());
    }
    
    // XOR with provided data
    for (size_t i = 0; i < std::min(temp.size(), provided_data.size()); ++i) {
        temp[i] ^= provided_data[i];
    }
    
    // Update key and counter
    std::copy(temp.begin(), temp.begin() + KEY_SIZE, key.begin());
    std::copy(temp.begin() + KEY_SIZE, temp.begin() + KEY_SIZE + BLOCK_SIZE, counter.begin());
}

std::vector<uint8_t> CTR_DRBG::generate(size_t num_bits) {
    size_t num_bytes = (num_bits + 7) / 8;
    std::vector<uint8_t> result;
    result.reserve(num_bytes);
    
    while (result.size() < num_bytes) {
        increment_counter();
        auto block = encrypt_block(counter);
        result.insert(result.end(), block.begin(), block.end());
    }
    
    result.resize(num_bytes);
    
    // Update state
    update({});
    reseed_counter++;
    
    return result;
}

void CTR_DRBG::reseed(const std::vector<uint8_t>& seed) {
    update(seed);
    reseed_counter = 1;
}

// ============================================================================
// Hash-DRBG Implementation
// ============================================================================

Hash_DRBG::Hash_DRBG(const std::vector<uint8_t>& seed) {
    // Hash_df to derive initial state
    V = hash_df(seed, SEED_LENGTH * 8);
    
    // Derive C
    std::vector<uint8_t> c_input = {0x00};
    c_input.insert(c_input.end(), V.begin(), V.end());
    C = hash_df(c_input, SEED_LENGTH * 8);
    
    reseed_counter = 1;
}

std::vector<uint8_t> Hash_DRBG::hash_df(const std::vector<uint8_t>& input, size_t no_of_bits) {
    size_t no_of_bytes = (no_of_bits + 7) / 8;
    size_t len = (no_of_bytes + HASH_OUTPUT - 1) / HASH_OUTPUT;
    
    std::vector<uint8_t> temp;
    uint8_t counter = 1;
    
    for (size_t i = 0; i < len; ++i) {
        std::vector<uint8_t> hash_input;
        hash_input.push_back(counter++);
        
        // no_of_bits as 32-bit big-endian
        hash_input.push_back(static_cast<uint8_t>((no_of_bits >> 24) & 0xFF));
        hash_input.push_back(static_cast<uint8_t>((no_of_bits >> 16) & 0xFF));
        hash_input.push_back(static_cast<uint8_t>((no_of_bits >> 8) & 0xFF));
        hash_input.push_back(static_cast<uint8_t>(no_of_bits & 0xFF));
        
        hash_input.insert(hash_input.end(), input.begin(), input.end());
        
        auto hash = sha256(hash_input);
        temp.insert(temp.end(), hash.begin(), hash.end());
    }
    
    temp.resize(no_of_bytes);
    return temp;
}

void Hash_DRBG::add_to_V(const std::vector<uint8_t>& value) {
    // Add value to V (modular addition treating as big integer)
    uint16_t carry = 0;
    size_t min_len = std::min(V.size(), value.size());
    
    for (size_t i = 0; i < V.size(); ++i) {
        size_t idx = V.size() - 1 - i;
        size_t val_idx = value.size() - 1 - i;
        
        uint16_t sum = static_cast<uint16_t>(V[idx]) + carry;
        if (i < min_len) {
            sum += static_cast<uint16_t>(value[val_idx]);
        }
        V[idx] = static_cast<uint8_t>(sum & 0xFF);
        carry = sum >> 8;
    }
}

std::vector<uint8_t> Hash_DRBG::hashgen(size_t requested_bits) {
    size_t m = (requested_bits + (HASH_OUTPUT * 8) - 1) / (HASH_OUTPUT * 8);
    std::vector<uint8_t> data = V;
    std::vector<uint8_t> W;
    
    for (size_t i = 0; i < m; ++i) {
        auto w = sha256(data);
        W.insert(W.end(), w.begin(), w.end());
        
        // Increment data
        for (int j = static_cast<int>(data.size()) - 1; j >= 0; --j) {
            if (++data[j] != 0) break;
        }
    }
    
    W.resize((requested_bits + 7) / 8);
    return W;
}

std::vector<uint8_t> Hash_DRBG::generate(size_t num_bits) {
    // Generate random bits
    auto returned_bits = hashgen(num_bits);
    
    // Update state
    std::vector<uint8_t> H_input = {0x03};
    H_input.insert(H_input.end(), V.begin(), V.end());
    auto H = sha256(H_input);
    
    // V = V + H + C + reseed_counter
    add_to_V(std::vector<uint8_t>(H.begin(), H.end()));
    add_to_V(C);
    
    // Add reseed_counter
    std::vector<uint8_t> rc_bytes(8);
    for (int i = 7; i >= 0; --i) {
        rc_bytes[7 - i] = static_cast<uint8_t>((reseed_counter >> (i * 8)) & 0xFF);
    }
    add_to_V(rc_bytes);
    
    reseed_counter++;
    
    return returned_bits;
}

void Hash_DRBG::reseed(const std::vector<uint8_t>& seed) {
    std::vector<uint8_t> seed_material = {0x01};
    seed_material.insert(seed_material.end(), V.begin(), V.end());
    seed_material.insert(seed_material.end(), seed.begin(), seed.end());
    
    V = hash_df(seed_material, SEED_LENGTH * 8);
    
    std::vector<uint8_t> c_input = {0x00};
    c_input.insert(c_input.end(), V.begin(), V.end());
    C = hash_df(c_input, SEED_LENGTH * 8);
    
    reseed_counter = 1;
}

// ============================================================================
// HMAC-DRBG Implementation
// ============================================================================

std::array<uint8_t, 32> HMAC_DRBG::hmac_sha256(const std::array<uint8_t, 32>& key,
                                                const std::vector<uint8_t>& data) {
    static constexpr size_t BLOCK_SIZE = 64;
    
    // Prepare key
    std::array<uint8_t, BLOCK_SIZE> k_pad = {};
    std::copy(key.begin(), key.end(), k_pad.begin());
    
    // Inner padding
    std::array<uint8_t, BLOCK_SIZE> i_key_pad;
    for (size_t i = 0; i < BLOCK_SIZE; ++i) {
        i_key_pad[i] = k_pad[i] ^ 0x36;
    }
    
    // Outer padding
    std::array<uint8_t, BLOCK_SIZE> o_key_pad;
    for (size_t i = 0; i < BLOCK_SIZE; ++i) {
        o_key_pad[i] = k_pad[i] ^ 0x5c;
    }
    
    // Inner hash
    std::vector<uint8_t> inner_data(i_key_pad.begin(), i_key_pad.end());
    inner_data.insert(inner_data.end(), data.begin(), data.end());
    auto inner_hash = sha256(inner_data);
    
    // Outer hash
    std::vector<uint8_t> outer_data(o_key_pad.begin(), o_key_pad.end());
    outer_data.insert(outer_data.end(), inner_hash.begin(), inner_hash.end());
    
    return sha256(outer_data);
}

HMAC_DRBG::HMAC_DRBG(const std::vector<uint8_t>& seed) {
    // Initial values
    K.fill(0x00);
    V.fill(0x01);
    reseed_counter = 1;
    
    // Update with seed
    update(seed);
}

void HMAC_DRBG::update(const std::vector<uint8_t>& provided_data) {
    // K = HMAC(K, V || 0x00 || provided_data)
    std::vector<uint8_t> temp(V.begin(), V.end());
    temp.push_back(0x00);
    temp.insert(temp.end(), provided_data.begin(), provided_data.end());
    K = hmac_sha256(K, temp);
    
    // V = HMAC(K, V)
    V = hmac_sha256(K, std::vector<uint8_t>(V.begin(), V.end()));
    
    if (!provided_data.empty()) {
        // K = HMAC(K, V || 0x01 || provided_data)
        temp.assign(V.begin(), V.end());
        temp.push_back(0x01);
        temp.insert(temp.end(), provided_data.begin(), provided_data.end());
        K = hmac_sha256(K, temp);
        
        // V = HMAC(K, V)
        V = hmac_sha256(K, std::vector<uint8_t>(V.begin(), V.end()));
    }
}

std::vector<uint8_t> HMAC_DRBG::generate(size_t num_bits) {
    size_t num_bytes = (num_bits + 7) / 8;
    std::vector<uint8_t> result;
    result.reserve(num_bytes);
    
    while (result.size() < num_bytes) {
        V = hmac_sha256(K, std::vector<uint8_t>(V.begin(), V.end()));
        result.insert(result.end(), V.begin(), V.end());
    }
    
    result.resize(num_bytes);
    
    // Update state
    update({});
    reseed_counter++;
    
    return result;
}

void HMAC_DRBG::reseed(const std::vector<uint8_t>& seed) {
    update(seed);
    reseed_counter = 1;
}
