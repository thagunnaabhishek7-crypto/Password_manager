#pragma once
#include <string>
#include <vector>
#include <ctime>

// GeneratorOptions struct to hold password generation options
enum class StrengthLevel {
    WEAK, FAIR, GOOD, STRONG, VERY_STRONG
};

std::string strengthLevelToString(StrengthLevel level);

enum class AuditAction {
    LOGIN, LOGOUT, ADD_ENTRY, UPDATE_ENTRY, DELETE_ENTRY, CHANGE_MASTER_PASSWORD, VAULT_CREATED
};


std::string auditActionToString(AuditAction action);
struct GeneratorOptions {
    int length = 16;
    bool useUppercase = true;
    bool useLowercase = true;
    bool useDigits = true;
    bool useSymbols = true;
    bool excludeAmbiguous = false;
};

// PasswordStrengthResult struct to hold the result of password strength evaluation
struct PasswordStrengthResult {
    int score;
    StrengthLevel level;
    std::vector<std::string> feedback;
    int entropyBits;
};


//Hashing and Salt Generation Class
class HashManager {
public:
    
    static std::string sha256(const std::string& input);
    static std::string generateSalt(int byteLength = 32);
    static std::string hashMasterPassword(const std::string& password, const std::string& salt);
    static bool verifyMasterPassword(const std::string& password, const std::string& salt, const std::string& storedHash);

private:
    
    static std::string bytesToHex(const unsigned char* bytes, size_t len);
};

// Encryption and Decryption Class
class EncryptionManager{
    private:
        std::string m_key;
        bool m_unlocked=false;
        static std::string bytesToHex(const unsigned char* bytes, size_t len);
        static std::vector<unsigned char> hexToBytes(const std::string& hex);
        static std::string toBase64(const std::vector<unsigned char>& data);
        static std::vector<unsigned char> fromBase64(const std::string& b64);
    public:
        static std::string deriveKey(const std::string& masterPassword, const std::string& salt, int iteration=100000);
        static std::string generateIV();
        static std::string encrypt(const std::string& plaintext, const std::string& key);
        static std::string decrypt(const std::string& b64Ciphertext, const std::string& key);
        void loadKey(const std::string& derivedKey);
        void clearKey();
        bool isMasterKeyLoaded() const;
        const std::string& getKey() const;


};