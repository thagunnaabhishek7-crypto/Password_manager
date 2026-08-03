#include "utilities.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include <stdexcept>
#include <cstdio>
#include <vector>

namespace {
int b64CharValue(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}
}

std::string EncryptionManager::bytesToHex(const unsigned char* bytes, size_t len) {
    std::string hex;
    hex.reserve(len * 2);

    char buf[3];
    for (size_t i = 0; i < len; ++i) {
        snprintf(buf, sizeof(buf), "%02x", bytes[i]);
        hex.append(buf, 2);
    }
    return hex;
}

std::vector<unsigned char> EncryptionManager::hexToBytes(const std::string& hex) {
    if (hex.size() % 2 != 0) {
        throw std::invalid_argument("Hex string must have even length");
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2) {
        unsigned int byte = 0;
        if (sscanf(hex.c_str() + i, "%2x", &byte) != 1) {
            throw std::invalid_argument("Invalid hex string");
        }
        bytes.push_back(static_cast<unsigned char>(byte));
    }
    return bytes;
}

std::string EncryptionManager::toBase64(const std::vector<unsigned char>& data) {
    static const char B64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    for (size_t i = 0; i < data.size(); i += 3) {
        unsigned int b0 = data[i];
        unsigned int b1 = (i + 1 < data.size()) ? data[i + 1] : 0;
        unsigned int b2 = (i + 2 < data.size()) ? data[i + 2] : 0;

        out += B64_CHARS[(b0 >> 2) & 0x3F];
        out += B64_CHARS[((b0 << 4) | (b1 >> 4)) & 0x3F];
        out += (i + 1 < data.size()) ? B64_CHARS[((b1 << 2) | (b2 >> 6)) & 0x3F] : '=';
        out += (i + 2 < data.size()) ? B64_CHARS[b2 & 0x3F] : '=';
    }
    return out;
}

std::vector<unsigned char> EncryptionManager::fromBase64(const std::string& b64) {
    std::vector<unsigned char> out;
    out.reserve((b64.size() / 4) * 3);

    int buffer = 0;
    int bits = 0;

    for (char c : b64) {
        if (c == '=') break;
        int val = b64CharValue(c);
        if (val < 0) continue;

        buffer = (buffer << 6) | val;
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<unsigned char>((buffer >> bits) & 0xFF));
        }
    }
    return out;
}

std::string EncryptionManager::deriveKey(const std::string& masterPassword, const std::string& salt, int iterations) {
    if (iterations <= 0) {
        throw std::invalid_argument("Iterations must be positive");
    }

    std::vector<unsigned char> saltBytes = hexToBytes(salt);
    std::vector<unsigned char> outKey(32);

    if (PKCS5_PBKDF2_HMAC(masterPassword.c_str(), static_cast<int>(masterPassword.size()),
                          saltBytes.data(), static_cast<int>(saltBytes.size()),
                          iterations, EVP_sha256(), 32, outKey.data()) != 1) {
        throw std::runtime_error("PBKDF2 key derivation failed");
    }

    return bytesToHex(outKey.data(), outKey.size());
}

std::string EncryptionManager::generateIV() {
    unsigned char iv[12];
    if (RAND_bytes(iv, sizeof(iv)) != 1) {
        throw std::runtime_error("RAND_bytes failed to generate IV");
    }
    return bytesToHex(iv, sizeof(iv));
}

std::string EncryptionManager::encrypt(const std::string& plaintext, const std::string& key) {
    std::vector<unsigned char> keyBytes = hexToBytes(key);
    std::string ivHex = generateIV();
    std::vector<unsigned char> ivBytes = hexToBytes(ivHex);

    if (keyBytes.size() != 32) {
        throw std::invalid_argument("Key must be 32 bytes (64 hex chars)");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to allocate EVP_CIPHER_CTX");
    }

    std::vector<unsigned char> ciphertext(plaintext.size() + 16);
    int len = 0;
    int total = 0;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, keyBytes.data(), ivBytes.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptInit_ex failed");
    }

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                          reinterpret_cast<const unsigned char*>(plaintext.data()),
                          static_cast<int>(plaintext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptUpdate failed");
    }
    total = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + total, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptFinal_ex failed");
    }
    total += len;

    unsigned char tag[16];
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to get GCM tag");
    }
    EVP_CIPHER_CTX_free(ctx);

    ciphertext.resize(total);

    std::vector<unsigned char> blob;
    blob.reserve(12 + ciphertext.size() + 16);
    blob.insert(blob.end(), ivBytes.begin(), ivBytes.end());
    blob.insert(blob.end(), ciphertext.begin(), ciphertext.end());
    blob.insert(blob.end(), tag, tag + 16);

    return toBase64(blob);
}

std::string EncryptionManager::decrypt(const std::string& b64Ciphertext, const std::string& key) {
    std::vector<unsigned char> keyBytes = hexToBytes(key);
    std::vector<unsigned char> blob = fromBase64(b64Ciphertext);

    if (blob.size() < 12 + 16) {
        throw std::runtime_error("Ciphertext blob too short");
    }

    std::vector<unsigned char> ivBytes(blob.begin(), blob.begin() + 12);
    std::vector<unsigned char> tag(blob.end() - 16, blob.end());
    std::vector<unsigned char> ciphertext(blob.begin() + 12, blob.end() - 16);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to allocate EVP_CIPHER_CTX");
    }

    std::vector<unsigned char> plaintext(ciphertext.size());
    int len = 0;
    int total = 0;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, keyBytes.data(), ivBytes.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptInit_ex failed");
    }

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                          ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptUpdate failed");
    }
    total = len;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set GCM tag");
    }

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + total, &len) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Decryption failed: authentication error");
    }
    total += len;
    EVP_CIPHER_CTX_free(ctx);

    plaintext.resize(total);
    return std::string(plaintext.begin(), plaintext.end());
}

void EncryptionManager::loadKey(const std::string& derivedKey) {
    m_key = derivedKey;
    m_unlocked = true;
}

void EncryptionManager::clearKey() {
    if (!m_key.empty()) {
        OPENSSL_cleanse(m_key.data(), m_key.size());
    }
    m_key.clear();
    m_unlocked = false;
}

bool EncryptionManager::isMasterKeyLoaded() const {
    return !m_key.empty();
}

const std::string& EncryptionManager::getKey() const {
    return m_key;
}