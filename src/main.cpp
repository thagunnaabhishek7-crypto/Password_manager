#include <iostream>
#include <vector>
#include "PasswordGenerator.h"
#include "utilities.h"

int main() 
{
    try 
    {
        std::vector<IPasswordGenerator*> generators;
        generators.push_back(new RandomPasswordGenerator());

        GeneratorOptions customOpts;
        customOpts.excludeAmbiguous = true;
        customOpts.length = 18;
        customOpts.useSymbols = false;
        generators.push_back(new RandomPasswordGenerator(customOpts));
        generators.push_back(new PassphraseGenerator(4));

        std::string encSalt = HashManager::generateSalt();
        
        std::string encKey = EncryptionManager::deriveKey("MyMasterPassword123", encSalt, 100000);
        
        std::string labels[] = {
            "Default Random Password   : ",
            "Custom Random Password    : ",
            "Passphrase (4 words)      : "
        };

        for (int i = 0; i < generators.size(); ++i) {
            std::string pwd = generators[i]->generate();
            std::string encryptedPwd = EncryptionManager::encrypt(pwd, encKey);
            std::cout << "\n" << labels[i] << pwd << std::endl;
            std::cout << "  ->>>>>>sha256 : " << HashManager::sha256(pwd) << std::endl;//hashing the given password using sha256
            std::cout << "  encrypted   : " << encryptedPwd << std::endl;
            std::cout << "  decrypted   : " << EncryptionManager::decrypt(encryptedPwd, encKey) << std::endl;

        }
        std::cout << std::endl;

        for (IPasswordGenerator* gen : generators) {
            delete gen;
        }
/*
        std::cout << "\n=== Encryption Tests ===\n";
        std::string salt = HashManager::generateSalt();
        std::string key = EncryptionManager::deriveKey("MyStr0ng!Pass", salt, 100000);
        std::cout << "Salt               : " << salt << std::endl;
        std::cout << "Derived key        : " << key << std::endl;

        std::string encrypted = EncryptionManager::encrypt("MySitePassword123", key);
        std::cout << "Encrypted          : " << encrypted << std::endl;

        std::string decrypted = EncryptionManager::decrypt(encrypted, key);
        std::cout << "Decrypted          : " << decrypted << std::endl;
        std::cout << "Round-trip         : " << (decrypted == "MySitePassword123" ? "OK" : "FAIL") << std::endl;

        std::string tampered = encrypted;
        tampered[0] = (tampered[0] == 'A') ? 'B' : 'A';
        try {
            EncryptionManager::decrypt(tampered, key);
            std::cout << "Tamper detected    : FAIL (no exception!)" << std::endl;
        } catch (const std::runtime_error& e) {
            std::cout << "Tamper detected    : OK -> " << e.what() << std::endl;
        }

        EncryptionManager enc;
        enc.loadKey(key);
        std::cout << "Key loaded         : " << (enc.isMasterKeyLoaded() ? "YES" : "NO") << std::endl;
        enc.clearKey();
        std::cout << "Key cleared        : " << (enc.isMasterKeyLoaded() ? "NO" : "YES") << std::endl;
*/
        /*std::cout << "\n=== Hashing Tests ===\n";
        std::cout << "sha256(\"abc\")      : " << HashManager::sha256("abc") << std::endl;
        std::cout << "Expected           : ba7816bf8f01cfea414140de5dae2ec73b00361bbef0469348423f656b05f15b" << std::endl;
        std::string salt = HashManager::generateSalt();
        std::string hash = HashManager::hashMasterPassword("MySecret123", salt);
        std::cout << "\nSalt               : " << salt << std::endl;
        std::cout << "Master hash        : " << hash << std::endl;
        std::cout << "Correct password   : " << (HashManager::verifyMasterPassword("MySecret123", salt, hash) ? "MATCH" : "FAIL") << std::endl;
        std::cout << "Wrong password     : " << (HashManager::verifyMasterPassword("WrongPass", salt, hash) ? "FAIL" : "REJECTED") << std::endl;
*/

    } 
    catch (const std::exception& e) 
    {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
