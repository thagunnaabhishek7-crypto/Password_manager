#pragma once 
#include <string>
#include <vector>
#include "utilities.h"

class IPasswordGenerator {
public:
    virtual ~IPasswordGenerator() = default;
    virtual std::string generate() = 0;
};


class RandomPasswordGenerator : public IPasswordGenerator {
private:
    GeneratorOptions m_options;

    static constexpr const char* UPPERCASE = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static constexpr const char* LOWERCASE = "abcdefghijklmnopqrstuvwxyz";
    static constexpr const char* DIGITS = "0123456789";
    static constexpr const char* SYMBOLS = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    static constexpr const char* AMBIGUOUS = "0O1lI";

    static std::string buildCharacterPool(const GeneratorOptions& options);
    static bool meetsOptions(const std::string& password, const GeneratorOptions& options);

public:
    RandomPasswordGenerator(const GeneratorOptions& options = GeneratorOptions());
    static int randomIndex(int max);
    std::string generate() override;
};

// Derived Class 2: Passphrase Generator
class PassphraseGenerator : public IPasswordGenerator {
private:
    int m_wordCount;
    static std::vector<std::string> loadWordList();

public:
    PassphraseGenerator(int wordCount = 4);
    std::string generate() override;
};
