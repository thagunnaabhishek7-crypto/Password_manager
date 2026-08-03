#include "utilities.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include <stdexcept>
#include <cstdio>
#include <vector>

// Convert bytes to hexadecimal string
std::string HashManager::bytesToHex(const unsigned char* bytes, size_t len) {
    std::string hex;
    hex.reserve(len * 2);
    
    char buf[3];
    for (size_t i = 0; i < len; ++i) {
        snprintf(buf, sizeof(buf), "%02x", bytes[i]);
        hex.append(buf, 2);
    }
    return hex;
}

// Compute SHA-256 hash of the input string
std::string HashManager::sha256(const std::string& input) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to allocate EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }

    if (EVP_DigestUpdate(ctx, input.data(), input.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestUpdate failed");
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int len = 0;

    if (EVP_DigestFinal_ex(ctx, digest, &len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }

    EVP_MD_CTX_free(ctx);
    return bytesToHex(digest, len);
}



// Generate a cryptographically secure random salt of specified byte length
std::string HashManager::generateSalt(int byteLength) {
    if (byteLength <= 0) {
        throw std::invalid_argument("Byte length must be positive");
    }

    std::vector<unsigned char> buffer(byteLength);
    if (RAND_bytes(buffer.data(), byteLength) != 1) {
        throw std::runtime_error("RAND_bytes failed to generate secure salt");
    }

    return bytesToHex(buffer.data(), buffer.size());
}



// Hash the master password with the provided salt using SHA-256
std::string HashManager::hashMasterPassword(const std::string& password, const std::string& salt) {
    return sha256(password + salt);
}



// Verify if the provided password matches the stored hash when combined with the salt
bool HashManager::verifyMasterPassword(const std::string& password, const std::string& salt, const std::string& storedHash) {
    std::string computedHash = hashMasterPassword(password, salt);

    if (computedHash.length() != storedHash.length()) {
        return false;
    }

    return (CRYPTO_memcmp(computedHash.data(), storedHash.data(), computedHash.length()) == 0);
}