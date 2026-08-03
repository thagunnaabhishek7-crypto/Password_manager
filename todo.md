# Password Manager — Implementation TODO
**Stack:** C++ · SFML · OpenSSL · nlohmann/json · Blockchain Audit Trail  
**Rule:** Check off each item only after it compiles and runs correctly. Never skip a phase — later phases depend on earlier ones.

---

## Phase 1 — Project Setup
**Goal:** A skeleton project that compiles with all three libraries before writing any logic.

- [ ] Create the full folder structure manually:
  ```
  PasswordManager/
  ├── CMakeLists.txt
  ├── include/utilities.h
  ├── src/main.cpp, utilities.cpp, HashManager.cpp,
  │       EncryptionManager.cpp, PasswordGenerator.cpp,
  │       PasswordStrengthChecker.cpp, Block.cpp, Blockchain.cpp,
  │       PasswordVault.cpp, VaultManager.cpp
  ├── ui/   (MainWindow, LoginDialog, AddEntryDialog,
  │          PasswordGeneratorWidget, StrengthMeterWidget,
  │          BlockchainViewerWidget — SFML-based, each has .cpp + .h)
  ├── data/   (empty; vault.enc / chain.json / config.json generated at runtime)
  ├── assets/wordlist.txt, common_passwords.txt, fonts/
  └── tests/test_all.cpp
  ```
- [ ] Write `CMakeLists.txt`:
  - `cmake_minimum_required(VERSION 3.16)`, `project(PasswordManager)`
  - `set(CMAKE_CXX_STANDARD 17)`
  - `find_package(SFML COMPONENTS window graphics system REQUIRED)`
  - `find_package(OpenSSL REQUIRED)`
  - `find_package(nlohmann_json REQUIRED)`
  - Link targets: `sfml-window`, `sfml-graphics`, `sfml-system`, `OpenSSL::SSL`, `OpenSSL::Crypto`, `nlohmann_json::nlohmann_json`
- [ ] Install libraries: OpenSSL dev headers, SFML 2.6+, nlohmann/json (via vcpkg, conan, or system package manager)
- [ ] Download a TTF font file (e.g.,arial.ttf) and place in `assets/fonts/` for SFML text rendering
- [ ] Smoke-test compile: `main.cpp` that includes `<SFML/Window.hpp>`, `<SFML/Graphics.hpp>`, `<openssl/sha.h>`, `<nlohmann/json.hpp>` — create a simple window and `return 0;`
- [ ] Confirm zero errors/warnings on the smoke-test build before proceeding

---

## Phase 2 — Core Data Structures
**File:** `include/utilities.h` (declarations) + `src/utilities.cpp` (toJson/fromJson bodies)  
**Rule:** No logic here — pure data. Get these right before touching any other file.

### `StrengthLevel` (enum class)
- [ ] Define values: `WEAK, FAIR, GOOD, STRONG, VERY_STRONG`
- [ ] Implement free function: `std::string strengthLevelToString(StrengthLevel level)` — returns display strings like `"WEAK"`, `"VERY STRONG"`

### `AuditAction` (enum class)
- [ ] Define values: `LOGIN, LOGOUT, ADD_ENTRY, UPDATE_ENTRY, DELETE_ENTRY, CHANGE_MASTER_PASSWORD, VAULT_CREATED`
- [ ] Implement free function: `std::string auditActionToString(AuditAction action)` — returns snake_case strings matching enum names

### `GeneratorOptions` (struct)
- [ ] `int length = 16`
- [ ] `bool useUppercase = true`
- [ ] `bool useLowercase = true`
- [ ] `bool useDigits = true`
- [ ] `bool useSymbols = true`
- [ ] `bool excludeAmbiguous = false` — strips chars `0`, `O`, `l`, `1`, `I` from pool

### `PasswordStrengthResult` (struct)
- [ ] `int score` — range 0–100
- [ ] `StrengthLevel level`
- [ ] `std::vector<std::string> feedback` — list of human-readable tips
- [ ] `int entropyBits`

### `PasswordEntry` (struct)
- [ ] `std::string id` — short unique ID, e.g. `"e3f7a1"` (6 hex chars)
- [ ] `std::string site`
- [ ] `std::string username`
- [ ] `std::string encryptedPassword` — AES-256-GCM ciphertext, base64-encoded
- [ ] `std::string category` — e.g. `"Social"`, `"Work"`, `"Finance"`
- [ ] `std::string notes`
- [ ] `std::time_t createdAt`
- [ ] `std::time_t updatedAt`
- [ ] `nlohmann::json toJson() const` — serialize all fields to JSON object
- [ ] `static PasswordEntry fromJson(const nlohmann::json& j)` — deserialize; use `.value("field", default)` to handle missing keys safely

### `BlockData` (struct)
- [ ] `AuditAction action`
- [ ] `std::string entryId` — ID of the affected entry; empty string for auth events
- [ ] `std::time_t timestamp`
- [ ] `std::string metadata` — JSON string for extra context (e.g. category name)
- [ ] `std::string toString() const` — returns one-liner like `"ADD_ENTRY | id:e3f7 | 2025-01-14 12:30"`

### `VaultConfig` (struct)
- [ ] `std::string vaultPath`
- [ ] `std::string blockchainPath`
- [ ] `std::string salt` — hex-encoded 32 random bytes
- [ ] `std::string masterHash` — SHA-256 of `(password + salt)`
- [ ] `int pbkdf2Iterations = 100000`
- [ ] `nlohmann::json toJson() const`
- [ ] `static VaultConfig fromJson(const nlohmann::json& j)`

---

## Phase 3 — Hashing Module
**File:** `src/HashManager.cpp`, declared in `include/utilities.h`  
**Dependency:** OpenSSL only. No other project code needed.

- [ ] `static std::string sha256(const std::string& input)`
  - Use `EVP_MD_CTX_new()` / `EVP_DigestInit_ex` / `EVP_DigestUpdate` / `EVP_DigestFinal_ex` / `EVP_MD_CTX_free()`
  - Pass raw `input.data()` and `input.size()` — do NOT cast to C-string (binary-safe)
  - Return result of `bytesToHex(digest, 32)` → 64-char lowercase hex string
- [ ] `static std::string generateSalt(int byteLength = 32)`
  - Call `RAND_bytes(buf, byteLength)` — check return value; throw on failure
  - Return `bytesToHex(buf, byteLength)`
- [ ] `static std::string hashMasterPassword(const std::string& password, const std::string& salt)`
  - Concatenate `password + salt`, pass to `sha256()`
  - Return the resulting hex hash
- [ ] `static bool verifyMasterPassword(const std::string& password, const std::string& salt, const std::string& storedHash)`
  - Re-hash with `hashMasterPassword(password, salt)`
  - Compare using `CRYPTO_memcmp(a.data(), b.data(), 64)` — **not** `==` (timing-safe)
  - Return `true` only if `CRYPTO_memcmp` returns 0 and lengths match
- [ ] `private static std::string bytesToHex(const unsigned char* bytes, size_t len)`
  - Loop `len` bytes: `snprintf` or `std::hex` stream each byte as 2-digit lowercase hex

---

## Phase 4 — Encryption Module
**File:** `src/EncryptionManager.cpp`  
**Dependencies:** OpenSSL, `HashManager` (for key derivation indirectly via PBKDF2)  
**Critical:** AES-256-GCM. The GCM authentication tag must be stored and verified — without it, tampered ciphertext will silently decrypt to garbage.

- [ ] `static std::string deriveKey(const std::string& masterPassword, const std::string& salt, int iterations = 100000)`
  - Convert hex salt back to raw bytes first
  - Call `PKCS5_PBKDF2_HMAC(password, passLen, salt_bytes, saltLen, iterations, EVP_sha256(), 32, outKey)`
  - Return `bytesToHex(outKey, 32)` — 64-char hex string representing the 256-bit key
- [ ] `static std::string generateIV()`
  - `RAND_bytes(iv, 12)` — 12 bytes for GCM
  - Return hex-encoded IV (24 chars)
- [ ] `static std::string encrypt(const std::string& plaintext, const std::string& key)`
  - Decode hex `key` → 32 raw bytes; call `generateIV()` → decode → 12 raw bytes
  - Init: `EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key_bytes, iv_bytes)`
  - Encrypt: `EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext_bytes, plaintext_len)`
  - Finalize: `EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)`
  - Get tag: `EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag)` — 16 bytes
  - Return `toBase64(IV_bytes + ciphertext_bytes + tag_bytes)` — concatenated in that order
- [ ] `static std::string decrypt(const std::string& b64Ciphertext, const std::string& key)`
  - `fromBase64()` the blob → raw bytes
  - Split: first 12 bytes = IV, last 16 bytes = tag, middle = ciphertext
  - Init: `EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key_bytes, iv_bytes)`
  - Set tag before finalizing: `EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag)`
  - Decrypt: `EVP_DecryptUpdate`, then `EVP_DecryptFinal_ex`
  - **If `EVP_DecryptFinal_ex` returns ≤ 0:** throw `std::runtime_error("Decryption failed: authentication error")`
- [ ] `void loadKey(const std::string& derivedKey)` — set `m_key = derivedKey`; `m_unlocked = true`
- [ ] `void clearKey()` — `OPENSSL_cleanse(m_key.data(), m_key.size())`; `m_key.clear()`; `m_unlocked = false`
- [ ] `bool isMasterKeyLoaded() const` — return `!m_key.empty()`
- [ ] `private static std::string toBase64(const std::vector<unsigned char>& data)` — use OpenSSL `BIO_f_base64` or manual table; no line breaks
- [ ] `private static std::vector<unsigned char> fromBase64(const std::string& b64)` — inverse; strip padding correctly

---

## Phase 5 — Password Generator
**File:** `src/PasswordGenerator.cpp`  
**Dependency:** OpenSSL (`RAND_bytes`), filesystem for wordlist

- [ ] `private static int randomIndex(int max)`
  - Draw random bytes; use rejection sampling: `if (rand_val >= (UINT_MAX - UINT_MAX % max)) retry`
  - **Do not** use `rand() % max` — modulo bias makes low indices more likely
- [ ] `private static std::string buildCharacterPool(const GeneratorOptions& opts)`
  - Concatenate enabled pools: `UPPERCASE`, `LOWERCASE`, `DIGITS`, `SYMBOLS`
  - If `opts.excludeAmbiguous`, erase all chars found in `AMBIGUOUS = "0O1lI"` from the pool
  - Throw if resulting pool is empty
- [ ] `private static bool meetsOptions(const std::string& pwd, const GeneratorOptions& opts)`
  - Check each enabled flag against the password — if `useUppercase` is true, ensure at least one uppercase char exists
  - Return false if any enabled character class is missing
- [ ] `std::string generate(const GeneratorOptions& opts)`
  - Build pool; loop until `meetsOptions` passes (avoids infinite loop: cap at 100 retries)
  - Fill array of `opts.length` chars using `randomIndex(pool.size())`
  - Fisher-Yates shuffle using `randomIndex` for each swap
  - Return as string
- [ ] `private static std::vector<std::string> loadWordList()`
  - Open `assets/wordlist.txt`; read one word per line into vector
  - If file missing or empty, fall back to embedded array of ~20 common EFF words
- [ ] `std::string generatePassphrase(int wordCount = 4)`
  - Load word list; pick `wordCount` words via `randomIndex(list.size())`
  - Join with `-`; return e.g. `"correct-horse-battery-staple"`

---

## Phase 6 — Password Strength Checker
**File:** `src/PasswordStrengthChecker.cpp`  
**Dependency:** `assets/common_passwords.txt` at runtime

- [ ] Implement all private predicates (each returns bool):
  - `hasUppercase(pwd)` — `std::any_of` with `std::isupper`
  - `hasLowercase(pwd)` — `std::any_of` with `std::islower`
  - `hasDigit(pwd)` — `std::any_of` with `std::isdigit`
  - `hasSymbol(pwd)` — check against SYMBOLS string manually
  - `hasMinLength(pwd, min=12)` — `pwd.size() >= min`
  - `hasNoRepeatingChars(pwd)` — fail if any char appears 3+ times consecutively (`"aaa"`)
  - `hasNoSequentialChars(pwd)` — fail if 3+ consecutive ASCII values (e.g. `'a','b','c'` or `'1','2','3'`)
  - `isCommonPassword(pwd)` — case-insensitive check against `COMMON_PASSWORDS`; load from file on first call (lazy init)
- [ ] `private static int getEntropyBits(const std::string& password)`
  - Determine charset size used (26 lower + 26 upper + 10 digits + 32 symbols as applicable)
  - Return `static_cast<int>(std::floor(std::log2(charsetSize) * password.size()))`
- [ ] `private static int calculateScore(const std::string& password)`
  - Apply scoring table (additive/subtractive, see original spec)
  - Clamp result: `std::max(0, std::min(100, score))`
- [ ] `private static StrengthLevel scoreToLevel(int score)`
  - `0–19 → WEAK | 20–39 → FAIR | 40–59 → GOOD | 60–79 → STRONG | 80+ → VERY_STRONG`
- [ ] `private static std::vector<std::string> collectFeedback(const std::string& pwd)`
  - For each failed rule, push a specific tip (e.g. `"Use at least 12 characters"`, `"Add symbols like !@#$"`, `"Avoid common passwords"`)
- [ ] `static PasswordStrengthResult check(const std::string& password)`
  - Call `calculateScore()`, `scoreToLevel()`, `collectFeedback()`, `getEntropyBits()`
  - Populate and return `PasswordStrengthResult`

---

## Phase 7 — Blockchain Audit Trail
**Files:** `src/Block.cpp`, `src/Blockchain.cpp`  
**Dependency:** `HashManager::sha256()`

### `Block`
- [ ] Constructor: `Block(int index, const BlockData& data, const std::string& previousHash)`
  - Set `m_index`, `m_data`, `m_previousHash`; `m_timestamp = time(nullptr)`; `m_nonce = 0`
  - Call `m_hash = calculateHash()` at end of constructor
- [ ] `std::string calculateHash() const`
  - Concatenate: `std::to_string(m_index) + std::to_string(m_timestamp) + m_data.toString() + m_previousHash + std::to_string(m_nonce)`
  - Return `HashManager::sha256(concatenated_string)`
- [ ] `void mineBlock(int difficulty = 2)`
  - Build `std::string target(difficulty, '0')` (e.g. `"00"`)
  - Loop: `m_nonce++; m_hash = calculateHash();` until `m_hash.substr(0, difficulty) == target`
  - Difficulty 2 is sufficient; do not set higher (performance)
- [ ] `std::string toJson() const` — serialize all fields to JSON string; use nlohmann::json
- [ ] `static Block fromJson(const std::string& jsonStr)` — parse JSON; reconstruct Block; **do not recalculate hash** (preserve stored hash)
- [ ] Getters for all private fields

### `Blockchain`
- [ ] Constructor: call `m_chain.push_back(createGenesisBlock())`
- [ ] `private Block createGenesisBlock()`
  - `BlockData genesisData{AuditAction::VAULT_CREATED, "", time(nullptr), ""}`
  - `Block(0, genesisData, "0")` — previousHash hardcoded `"0"`
  - Call `mineBlock()` on it; return
- [ ] `void addBlock(const BlockData& data)`
  - `Block b(m_chain.size(), data, getLatestBlock().getHash())`
  - `b.mineBlock()`
  - `m_chain.push_back(b)`
- [ ] `bool isChainValid() const`
  - For each block at index `i`: `if (block.getHash() != block.calculateHash()) return false`
  - For each block at index `i > 0`: `if (block.getPreviousHash() != m_chain[i-1].getHash()) return false`
  - Return `true` if all checks pass
- [ ] `std::vector<std::string> getAuditLog() const`
  - Map each block to string: `"[#" + index + "] " + formatted_timestamp + " | " + auditActionToString(action) + " | id:" + entryId`
- [ ] `bool saveToFile(const std::string& filename) const` — serialize `m_chain` to JSON array, write file
- [ ] `bool loadFromFile(const std::string& filename)` — read file, reconstruct chain, call `isChainValid()`, return false if invalid

---

## Phase 8 — Password Vault
**File:** `src/PasswordVault.cpp`  
**Dependency:** `EncryptionManager`, `PasswordEntry`

- [ ] `private static std::string generateId()`
  - `RAND_bytes(3 bytes)` → hex → 6-char string
  - Check `m_entries.count(id) == 0`; retry if collision (extremely rare but handle it)
- [ ] `bool addEntry(PasswordEntry& entry)`
  - Call `generateId()`; set `entry.id`
  - `entry.createdAt = entry.updatedAt = time(nullptr)`
  - `m_entries[entry.id] = entry`; return `true`
- [ ] `bool removeEntry(const std::string& id)` — erase from map; return `false` if not found
- [ ] `bool updateEntry(const std::string& id, const PasswordEntry& updated)`
  - Find entry; replace all fields except `id` and `createdAt`; set `updatedAt = time(nullptr)`
  - Return `false` if not found
- [ ] `std::optional<PasswordEntry> getEntry(const std::string& id) const` — return `std::nullopt` if missing
- [ ] `std::vector<PasswordEntry> searchEntries(const std::string& query) const`
  - Convert `query` to lowercase; check if any of `site`, `username`, `category`, `notes` contains it (case-insensitive)
  - Return matching entries as vector
- [ ] `std::vector<PasswordEntry> getAllEntries() const` — iterate `m_entries`, collect values
- [ ] `bool saveToFile(const std::string& filename, const EncryptionManager& enc) const`
  - Call `toJson()` to get JSON string
  - Call `EncryptionManager::encrypt(json_string, enc.getKey())` — add a `getKey()` accessor or pass key separately
  - Write ciphertext to file; return false on file error
- [ ] `bool loadFromFile(const std::string& filename, const EncryptionManager& enc)`
  - Read file contents as string
  - Call `EncryptionManager::decrypt(ciphertext, key)` — catch `std::runtime_error` and return false
  - Call `fromJson(plaintext)`
- [ ] `void clear()` — `m_entries.clear()`
- [ ] `private std::string toJson() const` — build nlohmann::json array from all entries
- [ ] `private bool fromJson(const std::string& json)` — parse array; call `PasswordEntry::fromJson()` on each element

---

## Phase 9 — Vault Manager (Orchestrator)
**File:** `src/VaultManager.cpp`  
**This is the only class the GUI talks to. Keep all file I/O and crypto invisible to UI code.**

- [ ] Private members: `HashManager m_hash`, `EncryptionManager m_encryption`, `PasswordGenerator m_generator`, `PasswordVault m_vault`, `Blockchain m_blockchain`, `VaultConfig m_config`, `std::string m_vaultDir`, `bool m_unlocked = false`
- [ ] Constructor: set `m_vaultDir`; derive paths `vault.enc`, `chain.json`, `config.json`
- [ ] `bool createVault(const std::string& masterPassword)`
  - `salt = HashManager::generateSalt()`
  - `masterHash = HashManager::hashMasterPassword(masterPassword, salt)`
  - Populate `m_config`; call `saveConfig()`
  - Create genesis block; save blockchain
  - Save empty encrypted vault (encrypt empty JSON array `"[]"`)
  - Return false on any file write error
- [ ] `bool unlock(const std::string& masterPassword)`
  - `loadConfig()`; verify password with `HashManager::verifyMasterPassword()`; return false on failure
  - `derivedKey = EncryptionManager::deriveKey(masterPassword, m_config.salt, m_config.pbkdf2Iterations)`
  - `m_encryption.loadKey(derivedKey)`
  - Load vault from file; load blockchain from file
  - `logAction(AuditAction::LOGIN)`; `m_unlocked = true`; return true
- [ ] `void lock()`
  - `m_vault.clear()`; `m_encryption.clearKey()`
  - `logAction(AuditAction::LOGOUT)`; save blockchain
  - `m_unlocked = false`
- [ ] `bool isUnlocked() const` — return `m_unlocked`
- [ ] `bool changeMasterPassword(const std::string& oldPwd, const std::string& newPwd)`
  - Verify old password; derive new key; re-encrypt entire vault with new key
  - Update `m_config` (new salt + hash); `saveConfig()`; `logAction(CHANGE_MASTER_PASSWORD)`
- [ ] `std::string addPassword(site, username, password, category = "")`
  - Encrypt raw password: `EncryptionManager::encrypt(password, m_encryption.getKey())`
  - Build `PasswordEntry`; call `m_vault.addEntry(entry)`; save vault
  - `logAction(AuditAction::ADD_ENTRY, entry.id)`; return `entry.id`
- [ ] `bool deletePassword(const std::string& id)` — remove from vault; save; log `DELETE_ENTRY`
- [ ] `bool updatePassword(const std::string& id, const std::string& newPassword)`
  - Re-encrypt; call `m_vault.updateEntry()`; save; log `UPDATE_ENTRY`
- [ ] `std::optional<std::string> getPassword(const std::string& id)`
  - Get entry; return `EncryptionManager::decrypt(entry.encryptedPassword, key)`; wrap in optional
- [ ] `std::vector<PasswordEntry> search(const std::string& query)` — delegate to `m_vault.searchEntries()`
- [ ] `std::string generateAndStore(site, username, const GeneratorOptions& opts)` — generate → addPassword → return plaintext password
- [ ] `std::string generatePreview(const GeneratorOptions& opts)` — generate only; do not store
- [ ] `PasswordStrengthResult checkStrength(const std::string& password)` — delegate to `PasswordStrengthChecker::check()`
- [ ] `std::vector<std::string> getAuditLog()` — delegate to `m_blockchain.getAuditLog()`
- [ ] `bool verifyAuditLog()` — delegate to `m_blockchain.isChainValid()`
- [ ] `private bool saveConfig() const` — serialize `m_config.toJson()` to `config.json`
- [ ] `private bool loadConfig()` — read `config.json`; parse with `VaultConfig::fromJson()`
- [ ] `private void logAction(AuditAction action, const std::string& entryId = "", const std::string& metadata = "")`
  - Build `BlockData`; `m_blockchain.addBlock(data)`; call `m_blockchain.saveToFile()`

---

## Phase 10 — GUI (SFML)
**Rule:** All vault operations go through `VaultManager`. Never call `EncryptionManager` or `HashManager` from UI code. Use SFML event polling instead of signal/slot mechanism.

### `LoginDialog` (sf::RenderWindow)
- [ ] Custom password input: `sf::String m_passwordInput` with `setEchoMode(true)` for masking
- [ ] `sf::Text` labels for "Password:" and error messages
- [ ] `sf::RectangleShape` buttons: "Unlock" and "Create Vault"
- [ ] Handle `sf::Event::TextEntered` for keyboard input
- [ ] Handle `sf::Event::KeyPressed` for Enter key to unlock
- [ ] `m_vaultManager.unlock(password)` called on button click
- [ ] Show error message in red if unlock fails
- [ ] `void vaultUnlocked()` callback to notify MainWindow

### `MainWindow` (sf::RenderWindow)
- [ ] Left sidebar: `sf::RectangleShape` buttons for "All Passwords", "Generator", "Audit Log", "Settings"
- [ ] Right panel: Custom table using `sf::RectangleShape` rows and `sf::Text` cells
- [ ] Table columns: `Site | Username | Category | Strength | Created | Actions`
- [ ] Strength column: colored rectangles (red/orange/yellow/green) based on `StrengthLevel`
- [ ] Action buttons: "Copy", "Edit", "Delete" per row using `sf::RectangleShape`
- [ ] Search bar: `sf::RectangleShape` with `sf::Text` input handling
- [ ] "Add" button opens `AddEntryDialog`
- [ ] "Lock" button calls `m_vaultManager.lock()` and shows `LoginDialog`
- [ ] Auto-lock: Track `sf::Clock` for inactivity; lock after 5 minutes
- [ ] Copy to clipboard: `sf::Clipboard::setString(password)`; auto-clear after 30s with `sf::Clock`
- [ ] Status bar: `sf::Text` showing "Unlocked — X entries"
- [ ] `void refreshTable()` — rebuild table from `VaultManager::getAllEntries()`

### `AddEntryDialog` (sf::RenderWindow)
- [ ] Input fields: `sf::String` for Site, Username, Password with `sf::Text` display
- [ ] Password mask toggle: Button to show/hide password characters
- [ ] Embed `StrengthMeterWidget` below password field
- [ ] "Generate" button opens `PasswordGeneratorWidget`
- [ ] "OK" button: Calls `VaultManager::addPassword()` or `updatePassword()`

### `PasswordGeneratorWidget` (sf::RenderWindow)
- [ ] Length slider: `sf::RectangleShape` with draggable handle (range 8-64, default 16)
- [ ] Checkboxes: `sf::RectangleShape` toggles for Uppercase, Lowercase, Digits, Symbols, Exclude Ambiguous
- [ ] Preview field: `sf::Text` showing generated password (read-only)
- [ ] "Regenerate" button: Calls `VaultManager::generatePreview()`
- [ ] "Copy" button: `sf::Clipboard::setString(preview)`
- [ ] "Use" button: Returns selected password to caller
- [ ] `void regenerate()` — rebuild preview from checkbox states

### `StrengthMeterWidget` (sf::Drawable)
- [ ] Progress bar: `sf::RectangleShape` (0-100 range)
- [ ] Level label: `sf::Text` showing "WEAK"/"FAIR"/"GOOD"/"STRONG"/"VERY STRONG"
- [ ] Feedback label: `sf::Text` with gray color, small font size
- [ ] Dynamic bar color: Set `sf::Color` based on score range (red/orange/yellow/green)
- [ ] Smooth animation: Lerp bar width over 300ms using `sf::Clock`
- [ ] `void updateStrength(const std::string& password)` — Call `VaultManager::checkStrength()`

### `BlockchainViewerWidget` (sf::RenderWindow)
- [ ] Table: `sf::RectangleShape` rows with `sf::Text` columns
- [ ] Columns: `# | Timestamp | Action | Entry ID | Hash (first 12 chars)`
- [ ] "Verify" button: Calls `VaultManager::verifyAuditLog()`
- [ ] Status label: `sf::Text` showing green "✓ Chain intact" or red "✗ TAMPERED!"
- [ ] `void refreshLog()` — Rebuild table from `VaultManager::getAuditLog()`

### SFML Helper Functions
- [ ] `void drawText(sf::RenderWindow& window, const std::string& text, sf::Vector2f position, sf::Color color, int size)`
- [ ] `void drawButton(sf::RenderWindow& window, sf::RectangleShape& button, sf::Text& label)`
- [ ] `bool isMouseOver(sf::RenderWindow& window, sf::RectangleShape& shape)`
- [ ] `void handleTextInput(sf::String& current, sf::Event& event, bool mask = false)`
- [ ] `sf::Color getStrengthColor(StrengthLevel level)` — Returns color for strength level

---

## Phase 11 — Testing
**File:** `tests/test_all.cpp`  
**No test framework required — use `assert()` and print pass/fail; or add Catch2/GoogleTest to CMake.**

- [ ] `HashManager::sha256("abc")` == `"ba7816bf8f01cfea414140de5dae2ec73b00361bbef0469348423f656b05f15"` (known SHA-256 vector)
- [ ] `HashManager::verifyMasterPassword(correct_pwd, salt, hash)` returns `true`; wrong password returns `false`
- [ ] `EncryptionManager::decrypt(encrypt(plain, key), key)` == `plain` for a known plaintext
- [ ] Tampered ciphertext (flip one byte) throws `std::runtime_error` in `decrypt()`
- [ ] `PasswordGenerator::generate(opts)` — verify all enabled character classes appear in output
- [ ] Run generator 1000 times; confirm no two outputs are identical
- [ ] `PasswordStrengthChecker::check("password")` → score ≤ 19, level `WEAK`
- [ ] `PasswordStrengthChecker::check("X#9mK!vQ2pLz")` → level `STRONG` or `VERY_STRONG`
- [ ] `PasswordStrengthChecker` feedback includes `"Add uppercase"` when password is all lowercase
- [ ] `Block::calculateHash()` returns identical result on two consecutive calls (deterministic)
- [ ] `Blockchain::isChainValid()` returns `true` on untouched chain
- [ ] Mutate `m_data` of a block directly; `isChainValid()` must return `false`
- [ ] `PasswordVault`: `addEntry()` → `getEntry()` → `removeEntry()` → `getEntry()` returns `nullopt`
- [ ] Integration: `VaultManager::createVault()` → `unlock()` → `addPassword()` → `getPassword()` returns original plaintext

---

## Phase 12 — Integration & Polish
**Only start this phase after all tests pass.**

- [ ] Wire `LoginDialog::vaultUnlocked()` callback → `MainWindow` constructor in `main.cpp`
- [ ] Ensure `VaultManager` instance is constructed once and passed by pointer/reference to all UI classes
- [ ] Test full flow: create vault → lock → unlock → add entry → copy password → check clipboard clears after 30s → auto-lock after 5 min
- [ ] Verify `data/chain.json` grows by one block per operation
- [ ] Test `BlockchainViewerWidget::verifyButton` shows green on a clean chain
- [ ] Manually edit `chain.json`, re-open app, verify tamper detection triggers
- [ ] Main SFML loop: Create `sf::RenderWindow`, handle events, draw UI, display

---

## Stretch Goals (post-completion, in priority order)
- [ ] **Password expiry warnings** — in `MainWindow::refreshTable()`, flag entries where `updatedAt < now - 90days` with an orange warning icon
- [ ] **Dark/Light theme toggle** — store color scheme in `config.json`; swap `sf::Color` values for UI elements
- [ ] **Export vault** — `VaultManager::exportToJson(path, masterPassword)` writes an encrypted JSON backup
- [ ] **Import from CSV** — parse `site,username,password` CSV; call `addPassword()` for each row; show import summary
- [ ] **System tray icon** — Use platform-specific API (e.g., Cocoa on macOS) for system tray; right-click menu with "Lock Vault" and "Open"
- [ ] **Ctrl+L shortcut** — Handle `sf::Event::KeyPressed` with `sf::Keyboard::L` + `sf::Keyboard::LControl` to lock vault

---

## Key Constraints & Gotchas
| Area | Rule |
|---|---|
| Crypto | Always use `EVP_*` API (OpenSSL 3.x). Never use deprecated `SHA256()` direct call. |
| GCM Tag | Must store and verify the 16-byte authentication tag. Skipping this makes AES-GCM equivalent to unauthenticated AES-CTR. |
| Key in memory | Call `EncryptionManager::clearKey()` on lock and on app close. Never log or print the derived key. |
| Timing safety | Use `CRYPTO_memcmp` for all hash comparisons. `==` on strings leaks timing info. |
| Modulo bias | Use rejection sampling in `randomIndex()`. Never `rand() % n`. |
| SFML events | Poll events in main loop using `window.pollEvent(event)`. Handle `sf::Event::Closed`, `KeyPressed`, `TextEntered`, `MouseButtonPressed`. |
| File paths | Use `std::filesystem::path` for path concatenation — never string concatenation with `/`. |
| JSON safety | Always use `.value("key", default)` in `fromJson()` — never `.at("key")` without a try/catch. |
| Thread safety | All vault operations run on the main thread. Do not move crypto to a worker thread without mutex protection. |
