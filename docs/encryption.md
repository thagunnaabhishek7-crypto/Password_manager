# Encryption Guide for Beginners (Phase 4)

## 1. Hashing vs Encryption — The Big Difference

You already know hashing (Phase 3). Now you need encryption. Here's the difference:

| | Hashing | Encryption |
|---|---|---|
| **Direction** | One-way (can't get input back) | Two-way (can decrypt) |
| **Purpose** | Verify passwords, integrity | Store secrets you need back |
| **Example** | `sha256("abc")` → `ba7816...` | `encrypt("mypass")` → `g4H2...` → `decrypt(...)` → `"mypass"` |
| **Used for** | Master password | Site passwords you must retrieve |

**In your project:**
- Master password → **hashed** (never need it back, just verify)
- Site passwords → **encrypted** (user clicks "show password" and needs it back)

---

## 2. What is AES-256-GCM?

### AES (Advanced Encryption Standard)
AES is a cipher — a recipe that scrambles data with a **key**.

- **AES-256** = uses a **256-bit (32-byte) key**. Bigger key = harder to crack.
- **Key example:** `f8a3c2b1e4d509aa...` (32 bytes → 64 hex chars)

### GCM (Galois/Counter Mode)
GCM is the "mode" — *how* AES scrambles. It has two jobs:
1. **Encrypt** — scramble the data (like hashing's little brother)
2. **Authenticate** — create a **tag** that proves nobody tampered with your data

**Why GCM matters:** If someone flips one byte in your file, GCM detects it on decrypt and **refuses to decrypt** (throws an error). Without GCM's tag, tampered data would silently decrypt into garbage.

### The GCM Tag (16 bytes)
- Created during **encryption**
- Stored alongside the ciphertext
- Checked during **decryption**
- If it doesn't match → `std::runtime_error("Decryption failed: authentication error")`

---

## 3. What You Need Before Encrypting

Encryption needs 3 things:

### 1. The Key (32 bytes)
Where does it come from? Your **master password** → **PBKDF2**.

```
master password "MyStr0ng!Pass" + salt "f7a3b2c1..."
        ↓  PBKDF2 (repeats 100,000 times)
   256-bit key  (32 bytes → 64 hex chars)
```

**PBKDF2 = Password-Based Key Derivation Function 2**
- Takes a weak password + salt
- Repeats the hashing **100,000 times** (slow on purpose)
- Why slow? Cracking a password requires running PBKDF2 100k times per guess — takes forever for attackers
- Output: a strong 256-bit key for AES

### 2. The IV (12 bytes)
**IV = Initialization Vector** (a.k.a. nonce)

- 12 random bytes, generated fresh for **EVERY encryption**
- Why? Same password + same key + same IV = identical ciphertext. An attacker could see two identical entries and know they share a password. Random IV fixes that.
- **Not secret** — stored with the ciphertext. It just has to be unique.

### 3. The Plaintext
The actual site password you want to protect.

---

## 4. The Encryption Flow (What Your Code Does)

```
encrypt("MySitePass123", key)
        ↓
    1. Generate 12-byte IV  (random)
    2. AES-256-GCM encrypt "MySitePass123" → ciphertext
    3. Get 16-byte auth tag
    4. Combine: IV + ciphertext + tag   (one blob)
    5. Base64 encode the blob
        ↓
   Returns: "x9tPq2...4kM="   ← what gets stored in the vault
```

### The Decryption Flow
```
decrypt("x9tPq2...4kM=", key)
        ↓
    1. Base64 decode → raw blob
    2. Split: first 12 bytes = IV, last 16 = tag, middle = ciphertext
    3. AES-256-GCM decrypt with key + IV
    4. Verify tag BEFORE accepting output
        ↓
   If tag OK  → returns "MySitePass123"  ✅
   If tag bad → throws error              ❌ (tampered!)
```

---

## 5. Why Base64?

Your encrypted output is **binary** (raw bytes). You can't store binary in a JSON file or show it on screen — it contains invisible/garbage characters.

**Base64** converts binary → safe text (letters, numbers, `+`, `/`, `=`).

```
Raw bytes:   0x1F  0x3A  0xE5  ...   (invisible garbage)
Base64:      HzplAq3s...             (readable text)
```

OpenSSL gives you `BIO_f_base64` for this, or you can write your own encoder.

---

## 6. What You'll Build (Phase 4 Code)

```
EncryptionManager
│
├── deriveKey(masterPassword, salt, iterations=100000) → 64-char hex key
├── generateIV()                                     → 24-char hex (12 bytes)
├── encrypt(plaintext, key)                          → base64 string
├── decrypt(b64Ciphertext, key)                      → plaintext string
├── loadKey(derivedKey)                              → sets m_key, m_unlocked
├── clearKey()                                       → OPENSSL_cleanse + clear
└── isMasterKeyLoaded()                              → bool
```

### OpenSSL Functions You Need

| Step | OpenSSL Function |
|---|---|
| Create cipher context | `EVP_CIPHER_CTX_new()` |
| Start encryption | `EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv)` |
| Feed data | `EVP_EncryptUpdate(ctx, out, &len, plaintext, plaintext_len)` |
| Finish | `EVP_EncryptFinal_ex(ctx, out + len, &len)` |
| Get tag (encrypt) | `EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag)` |
| Set tag (decrypt) | `EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag)` |
| Start decrypt | `EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv)` |
| Finish decrypt | `EVP_DecryptFinal_ex(ctx, out + len, &len)` |
| Derive key | `PKCS5_PBKDF2_HMAC(password, len, salt, saltLen, iters, EVP_sha256(), 32, outKey)` |
| Wipe key from memory | `OPENSSL_cleanse(data, size)` |

---

## 7. Key Rules (From the Spec)

| Rule | Why |
|---|---|
| **Store & verify the 16-byte GCM tag** | Without it, GCM is just plain AES-CTR — tampering undetected |
| **Set tag BEFORE `EVP_DecryptFinal_ex`** | Otherwise tag isn't checked |
| **Check `EVP_DecryptFinal_ex` return ≤ 0** | That's the authentication failure — throw immediately |
| **Fresh IV every encryption** | Reusing IV + same key = security leak |
| **`OPENSSL_cleanse` key on lock** | Never leave the key sitting in memory |
| **Never log the key** | It's the crown jewels |
| **Base64 without line breaks** | JSON/files need single-line strings |

---

## 8. How It All Fits Together (Vault Flow)

```
Create Vault:
  masterPassword → PBKDF2 → key (stored in EncryptionManager)
  site password "MySitePass123" → encrypt("MySitePass123", key)
  → store base64 ciphertext in PasswordEntry.encryptedPassword

Show Password:
  read encryptedPassword → decrypt(blob, key) → "MySitePass123" → show to user

Lock Vault:
  clearKey() → key gone from memory
  Now no one can decrypt anything until you unlock again
```

---

## 9. Test to Verify Your Code

```cpp
std::string key = EncryptionManager::deriveKey("MyStr0ng!Pass", salt, 100000);
std::string encrypted = EncryptionManager::encrypt("MySitePass123", key);
std::string decrypted = EncryptionManager::decrypt(encrypted, key);
// decrypted == "MySitePass123"  ✅

// Tamper test:
encrypted[5] = (encrypted[5] == 'A' ? 'B' : 'A');  // flip one base64 char
// decrypt(encrypted, key) MUST throw std::runtime_error
```

If decrypt throws on tampered data, your GCM is working correctly.

---

## 10. Why Is sha256 Output Different From Encrypted Text?

For the **same password**, `sha256()` and `encrypt()` produce very different outputs. They are doing completely different jobs:

### sha256 — Hashing (One-Way)
```
sha256("abc") → ba7816bf...   (ALWAYS the same)
sha256("abc") → ba7816bf...   (same every time)
```
- **Same password → always the same** 64-char hex output
- **Irreversible** — can't get the password back
- **No randomness** — pure deterministic math

### encrypt — Encryption (Reversible)
```
encrypt("abc", key) → iQUWyCDq...   (run 1)
encrypt("abc", key) → X9mK!vQ2p...  (run 2 — DIFFERENT!)
```
- **Same password → different output every run**
- **Reversible** — `decrypt()` recovers the original
- **Has randomness** — a fresh 12-byte IV mixed in each time

### Why the encrypted text changes every run
Each encryption gets a **fresh random IV** (12 bytes) mixed into the ciphertext. Since the IV is different each time, the ciphertext is different — even for the same password encrypted 100 times.

**Why this is a security feature:** If two vault entries had the same password, without a fresh IV they'd produce identical ciphertext. An attacker could see they share a password. The random IV hides that.

### Why sha256 never changes
Hashing has no IV and no randomness — it's deterministic math. Same input → same output, always.

### The different formats
| | sha256 | encrypt |
|---|---|---|
| **Format** | 64 hex chars (0-9, a-f) | base64 (A-Z, a-z, 0-9, +, /, =) |
| **Length** | always 64 | variable (depends on input length) |
| **Contains** | only hex digits | letters, digits, +, /, = |

### One-liner
**Hashing = one-way fingerprint (verify). Encryption = reversible vault (retrieve).** That's why their outputs look completely different.
