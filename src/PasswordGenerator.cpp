#include <openssl/rand.h>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <climits>
#include "PasswordGenerator.h"

RandomPasswordGenerator::RandomPasswordGenerator(const GeneratorOptions& options) 
    : m_options(options) {
}

int RandomPasswordGenerator::randomIndex(int max) {
    if (max <= 0) return 0;

    unsigned int rand_value;

    while (true) {
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&rand_value), sizeof(rand_value)) != 1) {
            throw std::runtime_error("Failed to generate random bytes");
        }

        if (rand_value < (UINT32_MAX - (UINT32_MAX % max))) {
            break;
        }
    }
    return static_cast<int>(rand_value % max);
}

std::string RandomPasswordGenerator::buildCharacterPool(const GeneratorOptions& options) {
    std::string pool = "";

    if (options.useUppercase) {
        pool += UPPERCASE;
    }
    if (options.useLowercase) {
        pool += LOWERCASE;
    }
    if (options.useDigits) {
        pool += DIGITS;
    }
    if (options.useSymbols) {
        pool += SYMBOLS;
    }

    if (options.excludeAmbiguous) {
        std::string ambiguousChars = AMBIGUOUS;
        for (char ch : ambiguousChars) {
            pool.erase(std::remove(pool.begin(), pool.end(), ch), pool.end());
        }
    }

    if (pool.empty()) {
        throw std::runtime_error("Character pool is empty");
    }

    return pool;
}

bool RandomPasswordGenerator::meetsOptions(const std::string& password, const GeneratorOptions& options) {
    if (options.useUppercase && password.find_first_of(UPPERCASE) == std::string::npos) {
        return false;
    }
    if (options.useLowercase && password.find_first_of(LOWERCASE) == std::string::npos) {
        return false;
    }
    if (options.useDigits && password.find_first_of(DIGITS) == std::string::npos) {
        return false;
    }
    if (options.useSymbols && password.find_first_of(SYMBOLS) == std::string::npos) {
        return false;
    }
    return true;
}

std::string RandomPasswordGenerator::generate() 
{
    std::string pool = buildCharacterPool(m_options);

    std::string password = ""; 
    bool generatedSuccessfully = false;
    for (int attempt = 0; attempt < 100; ++attempt) {
        password.clear();

        for (int i = 0; i < m_options.length; ++i) {
            int idx = randomIndex(static_cast<int>(pool.size()));
            password += pool[idx];
        }

        if (meetsOptions(password, m_options)) {
            generatedSuccessfully = true;
            break;
        }
    }

    if (!generatedSuccessfully) {
        throw std::runtime_error("Failed to generate password meeting options within 100 attempts");
    }

    for (int i = static_cast<int>(password.length()) - 1; i > 0; --i) {
        int j = randomIndex(i + 1);
        std::swap(password[i], password[j]);
    }
    return password;
}



PassphraseGenerator::PassphraseGenerator(int wordCount) 
    : m_wordCount(wordCount) {
}

std::vector<std::string> PassphraseGenerator::loadWordList() {
    std::vector<std::string> words;

    std::ifstream file("assets/wordlist.txt");

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                words.push_back(line);
            }
        }
        file.close();
    }

    if (words.empty()) {
        words = {
            "correct", "horse", "battery", "staple", "apple",
            "ocean", "rocket", "purple", "blue", "green",
            "mountain", "river", "sunset", "star", "moon",
            "cloud", "fire", "stone", "forest", "tree"
        };
    }

    return words;
}

std::string PassphraseGenerator::generate() {
    if (m_wordCount <= 0) {
        throw std::invalid_argument("Word count must be greater than 0");
    }

    std::vector<std::string> words = loadWordList();
    std::vector<std::string> selected;

    for (int i = 0; i < m_wordCount; ++i) {
        int idx = RandomPasswordGenerator::randomIndex(static_cast<int>(words.size()));
        selected.push_back(words[idx]);
    }

    std::string result = "";
    
    for (size_t i = 0; i < selected.size(); ++i) {
        result += selected[i];

        if (i < selected.size() - 1) {
            result += "-";
        }
    }
    return result;
}


