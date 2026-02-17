# 🔐 Advanced File Encryption System

![C](https://img.shields.io/badge/Language-C-blue?style=flat-square&logo=c)
![GTK](https://img.shields.io/badge/GUI-GTK4-green?style=flat-square)
![OpenSSL](https://img.shields.io/badge/Crypto-OpenSSL-red?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-yellow?style=flat-square)

A GUI-based file encryption and decryption desktop application built in **C** using **GTK4** for the graphical interface and **OpenSSL** for cryptographic operations. Supports **Caesar Cipher**, **XOR Cipher**, and **AES-256-GCM** encryption algorithms.

> **Course Code:** 0714 02 CSE 2100
> **Course Title:** Advanced Programming Laboratory  
> **Contributors:** MD. Abu Mursalin (ID: 240212). Udoy Munna (ID: 240229) 

---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [System Architecture](#system-architecture)
- [Algorithms](#algorithms)
- [What Changed — Full Refactoring Log](#what-changed--full-refactoring-log)
- [Folder Structure](#folder-structure)
- [Dependencies](#dependencies)
- [Building & Running](#building--running)
- [Usage Guide](#usage-guide)
- [Security Design](#security-design)
- [Design Patterns](#design-patterns)
- [SOLID Principles](#solid-principles)
- [Known Limitations](#known-limitations)
- [Future Improvements](#future-improvements)
- [Contributing](#contributing)

---

## Overview

This project started as an academic prototype and was refactored into a structured, industry-closer application by applying **SOLID principles**, **modular architecture**, **secure cryptographic practices**, and **clean coding standards**. The objective is to demonstrate practical software engineering skills including GUI development, cryptographic library integration, event-driven programming, and secure system design.

---

## Features

- 🗂️ Browse and select any file for encryption or decryption
- 🔒 Three encryption algorithms: Caesar Cipher, XOR Cipher, AES-256-GCM
- 🛡️ AES-256-GCM with authenticated encryption (tamper detection)
- 🔑 PBKDF2-HMAC-SHA256 key derivation with random salt (100,000 iterations)
- 🎲 Random IV generated per encryption for semantic security
- 👁️ Password visibility toggle (show/hide passphrase)
- ✅ Overwrite original file or save to a new path
- 🧹 One-click Clear to reset all fields
- 💬 Descriptive status and error messages for every operation
- 🔐 Secure key wiping from memory after use (`OPENSSL_cleanse`)

---

## System Architecture

The application is organised into four clean layers:

```
┌─────────────────────────────────────┐
│         UI Layer  (GTK4)            │  Window, buttons, labels, dialogs
├─────────────────────────────────────┤
│      Controller Layer               │  GTK signal callbacks, state transitions
├─────────────────────────────────────┤
│       Service Layer                 │  perform_encryption / perform_decryption
├─────────────────────────────────────┤
│       Crypto Layer                  │  caesar_encrypt/decrypt, xor_cipher,
│                                     │  aes_gcm_encrypt, aes_gcm_decrypt
└─────────────────────────────────────┘
           ↓
    File I/O  (fopen / fread / fwrite)
           ↓
    Encrypted / Decrypted Output File
```

### Key Structs

| Struct | Responsibility |
|--------|---------------|
| `AppState` | Owns all runtime state (paths, flags, action label). Replaces all global variables. |
| `WidgetBundle` | Groups every GTK widget pointer needed by callbacks. Passed via `gpointer user_data`. |

---

## Algorithms

### Caesar Cipher
A classical substitution cipher that shifts each alphabetic character by a fixed key value. Non-alphabetic characters are passed through unchanged.

- **Key range:** 1 – 25
- **Use case:** Educational / demonstration only. Not secure for sensitive data.

### XOR Cipher
Each byte of the file is XOR-ed with the key value. Because XOR is its own inverse, encryption and decryption use the same operation.

- **Key range:** 1 – 255
- **Use case:** Lightweight obfuscation. Not cryptographically secure on its own.

### AES-256-GCM *(primary secure algorithm)*
Industry-standard symmetric authenticated encryption. Uses a 256-bit key derived from the user's passphrase via PBKDF2. GCM mode provides both confidentiality and integrity — a wrong key or tampered file is detected and rejected before any output is written.

- **Key derivation:** PBKDF2-HMAC-SHA256, 100,000 iterations, 16-byte random salt
- **IV:** 12 bytes, randomly generated per encryption
- **Authentication tag:** 16 bytes (GCM tag), appended to ciphertext
- **Output file layout:** `[16B salt][12B IV][ciphertext][16B GCM tag]`

---

## What Changed — Full Refactoring Log

This section documents every change made during the refactoring of the original academic prototype into the current version.

---

### 🔴 Security — Critical Fixes

#### 1. AES mode upgraded from ECB to GCM

| | Before | After |
|-|--------|-------|
| **API** | Low-level `AES_encrypt()` / `AES_set_encrypt_key()` | EVP high-level API (`EVP_EncryptInit_ex`, `EVP_EncryptUpdate`, `EVP_EncryptFinal_ex`) |
| **Mode** | ECB (Electronic Codebook) — identical plaintext blocks produce identical ciphertext blocks | GCM (Galois/Counter Mode) — randomised, authenticated |
| **Integrity** | None — a tampered file decrypts silently to garbage | GCM authentication tag verified before writing any output |
| **Header** | `<openssl/aes.h>` | `<openssl/evp.h>` |

ECB mode is considered cryptographically broken for file encryption because patterns in the plaintext are visible in the ciphertext. GCM resolves this and adds integrity checking in a single step.

---

#### 2. Key derivation upgraded from SHA-256 to PBKDF2-HMAC-SHA256

| | Before | After |
|-|--------|-------|
| **Function** | `SHA256(password, len, key)` — single hash pass | `PKCS5_PBKDF2_HMAC(password, len, salt, salt_len, 100000, EVP_sha256(), 32, key)` |
| **Iterations** | 1 | 100,000 |
| **Salt** | None | 16 random bytes, stored in output file header |
| **Brute-force cost** | Negligible | ~100,000× more expensive per guess |

A bare `SHA256` call over a short password can be brute-forced at billions of guesses per second on modern hardware. PBKDF2 with 100,000 iterations and a unique random salt reduces this to thousands of guesses per second and eliminates pre-computed rainbow table attacks.

---

#### 3. Random IV added to AES encryption

**Before:** No IV was used at all — the same key always produced the same ciphertext for the same file.

**After:** `RAND_bytes(iv, 12)` generates a cryptographically secure 12-byte IV per encryption. The IV is written to the output file header and read back during decryption. This ensures semantic security — encrypting the same file twice with the same password produces completely different ciphertext.

---

#### 4. GCM authentication tag for integrity verification

**Before:** No integrity check. A corrupted or maliciously modified encrypted file would decrypt silently, possibly producing usable but incorrect data.

**After:** The GCM tag (16 bytes) is appended at the end of every encrypted file. During decryption, `EVP_DecryptFinal_ex` verifies the tag. If verification fails (wrong password, wrong algorithm, or tampered file), decryption is aborted and the user sees:
```
Authentication failed! Wrong key or file is corrupted.
```

---

#### 5. Secure key wiping after use

**Before:** The derived AES key lived in a stack buffer until the function returned. The compiler could optimise away a plain `memset`.

**After:** `OPENSSL_cleanse(aes_key, AES_KEY_SIZE)` is called in a `goto cleanup` path that runs regardless of success or failure. `OPENSSL_cleanse` is guaranteed not to be optimised away by the compiler.

---

### 🟠 Architecture — Structural Refactoring

#### 6. All global variables eliminated

**Before:** Six global variables scattered at file scope:
```c
FILE *fr, *fw;
char *path1, *path2, *Label;
bool fo, fs, ft;
```
These caused hidden coupling between functions, made testing impossible, and created race-condition risks.

**After:** All state lives in the `AppState` struct, heap-allocated in `activate()` and passed through `WidgetBundle`:
```c
typedef struct {
    char *input_path;
    char *output_path;
    char *action_label;
    bool  input_ready;
    bool  output_ready;
    bool  action_ready;
} AppState;
```

---

#### 7. Widget arrays replaced with a typed struct

**Before:** Multiple separate static arrays such as `widgets[6]`, `widgets1[6]`, `widgets2[3]`, `widgets3[10]`, `widgets4[4]`, `widgets5[2]` — all `GtkWidget *` — were declared and passed to different callbacks. These were error-prone and unreadable.

**After:** A single `WidgetBundle` struct holds every widget pointer by name. All callbacks receive a `WidgetBundle *` cast from `gpointer user_data`. Named fields replace magic array indices.

---

#### 8. Layered architecture enforced

**Before:** File I/O (`fopen`/`fclose`), key validation, and algorithm dispatch were all mixed inside the single `do_encrypt_decrypt` callback function.

**After:** Strict layer separation:
- `activate()` — UI construction only
- `on_*` / `browse_*` / `do_action` callbacks — controller logic only
- `perform_encryption()` / `perform_decryption()` — service layer (file I/O + dispatch)
- `caesar_encrypt()`, `xor_cipher()`, `aes_gcm_encrypt()`, etc. — crypto layer only

---

#### 9. Centralised ready-state logic

**Before:** The three-condition check (`ft && fo && fs`) and the placeholder-text update were copy-pasted into `on_toggled`, `on_response_open`, and `on_response_save` — three places to maintain.

**After:** A single `refresh_ready_state(WidgetBundle *wb)` function handles all readiness evaluation and UI updates. Every callback that can change state calls it once.

---

### 🟡 Error Handling — Comprehensive Validation

#### 10. Return values checked and surfaced to the user

**Before:** `fopen`, `fread`, `fwrite`, and OpenSSL calls had no error checking. A failed file open would lead to a null pointer dereference.

**After:** Every function in the service and crypto layers returns `bool` and accepts a `char *error_buf` / `size_t error_buf_size` pair. Specific, human-readable messages are written into the buffer and shown in the result label. Examples:

```
Cannot open input file: /path/to/file.txt
Failed to generate random salt/IV.
Authentication failed! Wrong key or file is corrupted.
File header is missing or corrupt.
Caesar key must be between 1 and 25.
```

---

#### 11. Input validation consolidated

**Before:** Key range validation for Caesar Cipher during decryption used the wrong limit (`key > 255` instead of `key > 25`), creating an inconsistency with the encryption path.

**After:** `validate_numeric_key()` centralises validation for both Caesar and XOR, for both encryption and decryption, eliminating the discrepancy.

---

### 🟢 Code Quality — Style & Maintainability

#### 12. Naming conventions — snake_case throughout

**Before:** Mixed naming: `caesar_cipher_encrypt`, `do_encrypt_decrypt`, `get_clear`, `browse_clicked_save`, `fo`, `fs`, `ft`, `fr`, `fw`.

**After:** Consistent `snake_case` with meaningful, self-documenting names:

| Old name | New name |
|----------|----------|
| `do_encrypt_decrypt` | `do_action` |
| `get_clear` | `on_clear` |
| `browse_clicked_open` | `browse_for_input` |
| `browse_clicked_save` | `browse_for_output` |
| `on_response_open` | `on_response_open` *(kept — already clear)* |
| `fo`, `fs`, `ft` | `input_ready`, `output_ready`, `action_ready` |
| `fr`, `fw` | `fr`, `fw` *(local only, not global)* |
| `path1`, `path2` | `output_path`, `input_path` |
| `Label` | `action_label` |

---

#### 13. AES algorithm label updated in the UI

**Before:** The combo box showed `"AES"` — ambiguous about mode.

**After:** The combo box shows `"AES-256-GCM"` — accurately describes the algorithm, key size, and mode, so users and reviewers know exactly what is being used.

---

#### 14. Memory management improved

**Before:** `path1` and `path2` were reassigned without freeing the previous value, leaking memory on every file selection.

**After:** `g_free(state->input_path)` and `g_free(state->output_path)` are called before each reassignment. The `on_clear` callback also frees both paths when resetting the UI.

---

#### 15. Streaming block size defined as a constant

**Before:** The AES functions read 16 bytes at a time (one cipher block), which is extremely slow for large files.

**After:** `FILE_BUFFER_SIZE 4096` processes 4 KB per EVP call. Because GCM is a stream-friendly mode, this works without padding concerns and is dramatically faster on large files.

---

#### 16. Single-file compilation simplified

The `FILE *fr` and `FILE *fw` are now **local variables** inside `perform_encryption` and `perform_decryption`. This means the functions are fully reentrant and their lifetimes are tightly scoped — files are opened and closed within the same function call.

---

## Folder Structure

```
project_root/
├── src/
│   └── file_encryptor.c      # Full application source
├── include/                  # (future: split headers here)
│   ├── encryption.h
│   └── file_handler.h
├── tests/                    # (future: unit tests)
├── docs/                     # Project documentation
│   ├── Advanced_File_Encryption_With_Diagrams.docx
│   ├── Advanced_File_Encryption_System_Final_Report.docx
│   └── Advanced_File_Encryption_Thesis_Level_Report.docx
├── README.md
└── Makefile
```

---

## Dependencies

| Dependency | Purpose | Install |
|------------|---------|---------|
| GTK 4 | GUI framework | `sudo apt install libgtk-4-dev` |
| OpenSSL ≥ 1.1 | AES-256-GCM, PBKDF2, RAND_bytes | `sudo apt install libssl-dev` |
| GCC | C compiler | `sudo apt install gcc` |
| Make | Build automation | `sudo apt install make` |

---

## Building & Running

### Using GCC directly

```bash
gcc file_encryptor.c -o file_encryptor \
    $(pkg-config --cflags --libs gtk4) \
    -lssl -lcrypto \
    -Wall -Wextra -O2
```

### Using Make (recommended)

Create a `Makefile`:

```makefile
CC      = gcc
CFLAGS  = $(shell pkg-config --cflags gtk4) -Wall -Wextra -O2
LIBS    = $(shell pkg-config --libs gtk4) -lssl -lcrypto
TARGET  = file_encryptor
SRC     = src/file_encryptor.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)
```

Then build and run:

```bash
make
./file_encryptor
```

---

## Usage Guide

1. **Select Input File** — Click *Browse…* next to "File" and choose the file you want to encrypt or decrypt.
2. **Select Output File** — Click *Browse…* next to "Output Folder" and choose where the result will be saved. To overwrite the original, select the same file — the *Overwrite original* checkbox will tick automatically.
3. **Choose Algorithm** — Select one from the dropdown: *Caesar Cipher*, *Xor Cipher*, or *AES-256-GCM*.
4. **Select Action** — Choose *Encrypt* or *Decrypt* using the radio buttons.
5. **Status turns Ready** — Once all three steps above are complete, the status label reads `Status : Ready` and the key field becomes editable.
6. **Enter Key or Passphrase:**
   - Caesar Cipher: enter a number 1–25
   - XOR Cipher: enter a number 1–255
   - AES-256-GCM: enter any passphrase (longer is stronger)
7. **Click Encrypt / Decrypt** — The action button label reflects your selection. The result label reports success or a specific error.
8. **Clear** — Resets all fields to their default state.

> **⚠️ Important:** AES-256-GCM encrypted files can only be decrypted with the exact same passphrase. There is no recovery mechanism. Store your passphrase securely.

---

## Security Design

### Encrypted File Format (AES-256-GCM)

```
┌──────────────────────┐
│  16 bytes  PBKDF2    │  Random salt — unique per encryption
│            salt      │
├──────────────────────┤
│  12 bytes  IV        │  Random IV — unique per encryption
├──────────────────────┤
│  N  bytes  Cipher-   │  AES-256-GCM encrypted file content
│            text      │
├──────────────────────┤
│  16 bytes  GCM Tag   │  Authentication tag — detects tampering
└──────────────────────┘
```

### Key Derivation

```
passphrase  ──┐
              ├──→ PBKDF2-HMAC-SHA256 (100,000 iterations) ──→ 256-bit AES key
random salt ──┘
```

### Threat Model Coverage

| Threat | Mitigation |
|--------|-----------|
| Brute-force password attack | PBKDF2 with 100k iterations + unique salt |
| Rainbow table attack | Unique random salt per encryption |
| Ciphertext-only analysis | AES-256-GCM (IND-CCA2 secure) |
| File tampering / corruption | GCM authentication tag verification |
| Key left in memory | `OPENSSL_cleanse` after use |
| IV reuse | Randomly generated IV per encryption |

---

## Design Patterns

| Pattern | Where Applied |
|---------|--------------|
| **Strategy Pattern** | Each algorithm (`caesar_encrypt`, `xor_cipher`, `aes_gcm_encrypt`) is an interchangeable function matching the same interface, selected at runtime by `perform_encryption`. |
| **Factory Pattern** | `perform_encryption` / `perform_decryption` act as factories that dispatch to the correct algorithm based on the selected algorithm string. |
| **Singleton Pattern** | `AppState` is created once in `activate()` and shared across all callbacks via `WidgetBundle`. |
| **Observer Pattern** | GTK's signal system (`g_signal_connect`) implements observer — UI events notify callbacks without tight coupling. |

---

## SOLID Principles

| Principle | Implementation |
|-----------|---------------|
| **Single Responsibility** | `caesar_encrypt` encrypts only. `perform_encryption` handles file I/O and dispatch only. `refresh_ready_state` updates UI state only. |
| **Open / Closed** | Adding a new algorithm requires only a new crypto function and one new `else if` branch in `perform_encryption` / `perform_decryption` — existing code is untouched. |
| **Liskov Substitution** | All cipher functions share compatible signatures `(FILE *fr, FILE *fw, ...)` and can be substituted for one another in the dispatch layer. |
| **Interface Segregation** | The crypto layer has no knowledge of GTK. The UI layer has no knowledge of OpenSSL. |
| **Dependency Inversion** | The service layer (`perform_encryption`) depends on the abstract concept of "a function that transforms bytes", not on any specific algorithm implementation. |

---

## Known Limitations

1. AES-256-GCM loads the full ciphertext into a seek-able buffer to extract the trailing tag — very large files (>1 GB) may use significant memory. A future improvement would use a two-pass or length-prefixed approach.
2. Caesar and XOR ciphers are not cryptographically secure and are included for educational demonstration only.
3. No logging or audit trail for encryption operations.
4. No automated test suite yet.
5. No progress bar for large file operations.
6. Cross-platform support is Linux-focused; Windows requires MSYS2 or WSL.

---

## Future Improvements

- [ ] Add RSA public-key encryption for key exchange
- [ ] Implement a progress bar for large file encryption
- [ ] Add file drag-and-drop support
- [ ] Introduce a logging framework for audit trails
- [ ] Add unit tests with a C testing framework (e.g., CUnit, cmocka)
- [ ] Split into separate `.c` / `.h` modules (`encryption.c`, `file_handler.c`, etc.)
- [ ] Multi-threading for non-blocking UI during heavy encryption
- [ ] Cloud storage integration (upload encrypted output)
- [ ] CMake build system for cross-platform support
- [ ] Hardware Security Module (HSM) integration for key storage
- [ ] Post-quantum cryptography readiness (CRYSTALS-Kyber)

---

## Contributing

This project follows a **GitFlow** branching strategy:

| Branch | Purpose |
|--------|---------|
| `main` | Production-ready, stable releases |
| `develop` | Integration branch for features |
| `feature/*` | Individual feature development |
| `release/*` | Release preparation and testing |
| `hotfix/*` | Urgent production fixes |

**Workflow:**
1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature-name`
3. Commit with descriptive messages: `git commit -m "feat: add PBKDF2 key derivation"`
4. Push and open a Pull Request against `develop`
5. Ensure the code compiles cleanly with `-Wall -Wextra` before submitting

---

*Advanced File Encryption System — Advanced Programming Techniques · 2026*
