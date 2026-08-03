# Hashing Guide for Beginners

## 1. What is Hashing? (Analogy)

Imagine a **name stamp** that creates a **fingerprint** of whatever you type.

You feed in text → out comes a fixed-length code:
```
sha256("hello") → 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824
sha256("Hello") → 185f8db32271fe25f561a6fc938b2e264306ec304eda518007d1764826381969
```

Key point: **"hello" and "Hello" produce completely different outputs** despite being 1 character apart.

### Think of it Like a Blender
- Put in an apple → you get a smoothie (can't get the apple back)
- Put in an apple again → you get the exact same smoothie
- Put in an apple with a tiny piece of banana → completely different smoothie

That's hashing:
- **One-way:** You can't reverse the smoothie back to the apple
- **Deterministic:** Same input → same output every time
- **Avalanche effect:** Small change → wildly different output

---

## 2. Why Do We Need Hashing in This Project?

### Problem: Storing Passwords
If you store passwords as plain text and someone breaks in, they have **all passwords**.

### Solution: Hash Instead
```
User creates password: "MyP@ss123"
You store:            sha256("MyP@ss123") = "4f7b3a..."

When user logs in:
  They type "MyP@ss123"
  You hash it: sha256("MyP@ss123") = "4f7b3a..."
  Compare with stored hash → matches! ✅
```

Even if hackers steal your database, they only have **meaningless hash strings**, not real passwords.

---

## 3. What is SHA-256? (The Recipe)

SHA-256 is a specific hashing "recipe" that produces a 256-bit (32 byte) result.

Your output will be **64 characters long** (because each byte = 2 hex characters):
```
sha256("abc") = "ba7816bf8f01cfea414140de5dae2ec73b00361bbef0469348423f656b05f15b"
               ↑ 64 characters total ↑
```

---

## 4. What's This "Hex" Thing?

Computers work with **bytes** (numbers 0-255). Humans can't read raw bytes easily:
```
Raw bytes: 186 120 022 191 ...
```
So we convert each byte to **2 hex characters**:
```
186 → "ba"    120 → "78"    022 → "16"    191 → "bf"
```

That's why your 32-byte hash becomes a 64-character hex string.

---

## 5. What is a Salt? (Why Randomness Matters)

A **salt** is just random data you attach to the password before hashing.

### Without Salt (BAD)
```
User1 password: "password123" → sha256("password123") → 6c23...
User2 password: "password123" → sha256("password123") → 6c23...  ← SAME!
```
Both users have the exact same hash. If a hacker cracks one, they know the other's password too.

### With Salt (GOOD)
```
User1: salt = "a4f8..." → sha256("password123" + "a4f8...") → 9b71...
User2: salt = "7c2e..." → sha256("password123" + "7c2e...") → da45...  ← DIFFERENT!
```
Same password → totally different hashes because each user gets a unique random salt.

### How Salt Protects You
| Attack | Without Salt | With Salt |
|--------|-------------|-----------|
| **Rainbow table** (pre-computed hash list) | Attacker looks up hash → finds password instantly | Useless — hash depends on random salt |
| **Same passwords** | Both users have identical stored hashes | Hashes are different |
| **Cracking speed** | Hash once per guess | Must re-hash for EVERY user's salt |

---

## 6. The Full Flow (Password Manager)

### Creating a Vault (First Time)
```
1. You type master password: "MyStr0ng!Pass"
2. Program creates salt:     "f7a3b2c1..."  (32 random bytes → hex)
3. Program hashes:           sha256("MyStr0ng!Pass" + "f7a3b2c1...") = "8d4e..."
4. Stores:  { salt: "f7a3b2c1...", hash: "8d4e..." }
   (never stores the actual password)
```

### Unlocking (Logging In)
```
1. You type: "MyStr0ng!Pass"
2. Program reads stored salt: "f7a3b2c1..."
3. Program hashes:            sha256("MyStr0ng!Pass" + "f7a3b2c1...")
4. Compares with stored "8d4e..."
   - Match ✅ → You're in!
   - No match ❌ → Wrong password
```

---

## 7. Timing Attack — Why `==` is Dangerous

### Normal people think:
```cpp
if (computedHash == storedHash)  // Looks fine, right?
```

### But hackers know:
`==` stops comparing **the moment it finds a difference**.

If the first byte differs, it stops after checking 1 byte (fast).
If the first 5 bytes match but the 6th differs, it checks 6 bytes (slower).

A hacker types a guess, measures how long it takes, and knows **how many bytes were correct**. After thousands of attempts, they can figure out every single byte of your hash.

### The Fix: Constant-Time Comparison
```cpp
CRYPTO_memcmp(a.data(), b.data(), 64)  // Always checks all 64 bytes
```
Same time whether 0 bytes match or 63 bytes match. No timing leak.

---

## 8. Code Structure for Phase 3

You'll create a file `src/HashManager.cpp` with these functions:

```
HashManager
├── sha256("hello")              → "2cf24dba..."  (64 char hex)
├── generateSalt(32)             → "f7a3b2c1..."  (random hex)
├── hashMasterPassword(pwd, salt) → sha256(pwd + salt)
├── verifyMasterPassword(pwd, salt, hash) → true/false
└── bytesToHex(bytes, len)       → hex string (private helper)
```

### Which OpenSSL functions to call
| What you need | OpenSSL function |
|---|---|
| Create a hashing engine | `EVP_MD_CTX_new()` |
| Set it to SHA-256 | `EVP_DigestInit_ex(ctx, EVP_sha256(), NULL)` |
| Feed data into it | `EVP_DigestUpdate(ctx, data, len)` |
| Get the final hash | `EVP_DigestFinal_ex(ctx, digest, &len)` |
| Clean up | `EVP_MD_CTX_free(ctx)` |
| Generate random bytes (for salt) | `RAND_bytes(buf, len)` |
| Compare hashes safely | `CRYPTO_memcmp(a, b, len)` |

### Three Rules You MUST Follow
1. **Always use EVP_* functions** — there's an older `SHA256()` function that's deprecated and may be removed
2. **Always check RAND_bytes return value** — if it fails and you don't check, your salt might be garbage
3. **Always use CRYPTO_memcmp for hash comparison** — never `==`

---

## 9. Known Test (To Verify Your Code Works)

```cpp
HashManager::sha256("abc")
// Must equal:
"ba7816bf8f01cfea414140de5dae2ec73b00361bbef0469348423f656b05f15b"
```

If your output matches this, SHA-256 is implemented correctly.
