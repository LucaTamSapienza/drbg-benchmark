/**
 * @file drbg.hpp
 * @brief Abstract base class and implementations for Deterministic Random Bit Generators (DRBG)
 * 
 * This file contains implementations of three CS-PRNG algorithms:
 * 1. CTR-DRBG (Counter mode DRBG) - Based on AES-like block cipher
 * 2. Hash-DRBG - Based on SHA-256 hash function
 * 3. HMAC-DRBG - Based on HMAC-SHA256
 */

#ifndef DRBG_HPP
#define DRBG_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <cstring>

/**
 * @class DRBG
 * @brief Abstract base class for all DRBG implementations
 */
class DRBG {
public:
    virtual ~DRBG() = default;
    
    /**
     * @brief Generate random bits
     * @param num_bits Number of bits to generate
     * @return Vector of bytes containing the random bits
     */
    virtual std::vector<uint8_t> generate(size_t num_bits) = 0;
    
    /**
     * @brief Reseed the DRBG with new entropy
     * @param seed New seed data
     */
    virtual void reseed(const std::vector<uint8_t>& seed) = 0;
    
    /**
     * @brief Get the name of this DRBG implementation
     * @return Name string
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief Get the internal state size in bytes
     * @return State size
     */
    virtual size_t getStateSize() const = 0;
};

/**
 * @class CTR_DRBG
 * @brief Counter-mode DRBG based on a simplified AES-like block cipher
 * 
 * CTR-DRBG uses a block cipher in counter mode. We implement a simplified
 * version based on AES principles for educational purposes.
 */
class CTR_DRBG : public DRBG {
private:
    static constexpr size_t BLOCK_SIZE = 16;  // 128-bit blocks
    static constexpr size_t KEY_SIZE = 32;    // 256-bit key
    
    std::array<uint8_t, KEY_SIZE> key;
    std::array<uint8_t, BLOCK_SIZE> counter;
    uint64_t reseed_counter;
    
    // Simplified block cipher (SPN-based)
    std::array<uint8_t, BLOCK_SIZE> encrypt_block(const std::array<uint8_t, BLOCK_SIZE>& block);
    void increment_counter();
    void update(const std::vector<uint8_t>& provided_data);
    
    // SPN components
    static constexpr uint8_t SBOX[256] = {
        0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
        0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
        0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
        0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
        0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
        0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
        0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
        0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
        0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
        0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
        0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
        0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
        0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
        0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
        0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
        0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
    };

public:
    explicit CTR_DRBG(const std::vector<uint8_t>& seed);
    std::vector<uint8_t> generate(size_t num_bits) override;
    void reseed(const std::vector<uint8_t>& seed) override;
    std::string getName() const override { return "CTR-DRBG"; }
    size_t getStateSize() const override { return KEY_SIZE + BLOCK_SIZE + sizeof(reseed_counter); }
};

/**
 * @class Hash_DRBG
 * @brief Hash-based DRBG using SHA-256
 * 
 * Implements NIST SP 800-90A Hash_DRBG construction using SHA-256
 */
class Hash_DRBG : public DRBG {
public:
    // SHA-256 implementation (public for use by HMAC_DRBG)
    static std::array<uint8_t, 32> sha256(const std::vector<uint8_t>& data);

private:
    static constexpr size_t SEED_LENGTH = 55;  // seedlen for SHA-256 (440 bits)
    static constexpr size_t HASH_OUTPUT = 32;  // SHA-256 output size
    
    std::vector<uint8_t> V;  // Internal state
    std::vector<uint8_t> C;  // Constant value
    uint64_t reseed_counter;
    std::vector<uint8_t> hash_df(const std::vector<uint8_t>& input, size_t no_of_bits);
    std::vector<uint8_t> hashgen(size_t requested_bits);
    void add_to_V(const std::vector<uint8_t>& value);

public:
    explicit Hash_DRBG(const std::vector<uint8_t>& seed);
    std::vector<uint8_t> generate(size_t num_bits) override;
    void reseed(const std::vector<uint8_t>& seed) override;
    std::string getName() const override { return "Hash-DRBG"; }
    size_t getStateSize() const override { return V.size() + C.size() + sizeof(reseed_counter); }
};

/**
 * @class HMAC_DRBG
 * @brief HMAC-based DRBG using HMAC-SHA256
 * 
 * Implements NIST SP 800-90A HMAC_DRBG construction
 */
class HMAC_DRBG : public DRBG {
private:
    static constexpr size_t HASH_OUTPUT = 32;  // SHA-256/HMAC output size
    
    std::array<uint8_t, HASH_OUTPUT> K;  // Key
    std::array<uint8_t, HASH_OUTPUT> V;  // Value
    uint64_t reseed_counter;
    
    // HMAC-SHA256 implementation
    static std::array<uint8_t, 32> hmac_sha256(const std::array<uint8_t, 32>& key, 
                                                const std::vector<uint8_t>& data);
    static std::array<uint8_t, 32> sha256(const std::vector<uint8_t>& data);
    void update(const std::vector<uint8_t>& provided_data);

public:
    explicit HMAC_DRBG(const std::vector<uint8_t>& seed);
    std::vector<uint8_t> generate(size_t num_bits) override;
    void reseed(const std::vector<uint8_t>& seed) override;
    std::string getName() const override { return "HMAC-DRBG"; }
    size_t getStateSize() const override { return sizeof(K) + sizeof(V) + sizeof(reseed_counter); }
};

#endif // DRBG_HPP
