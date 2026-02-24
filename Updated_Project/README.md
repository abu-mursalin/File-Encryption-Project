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

This project started as an academic prototype and was progressively refactored into a structured, industry-closer application by applying **SOLID principles**, **modular architecture**, **secure cryptographic practices**, and **clean coding standards**. This updated version introduces a complete modular split into 8 focused source files, a timestamped logging system, a critical AES decryption bug fix, and full Windows/MinGW compatibility. The objective is to demonstrate practical software engineering skills including GUI development, cryptographic library integration, event-driven programming, and secure system design.

---

## Features

- 🗂️ Browse and select any file for encryption or decryption
- 🔒 Three encryption algorithms: Caesar Cipher, XOR Cipher, AES-256-GCM
- 🛡️ AES-256-GCM with authenticated encryption (tamper detection)
- 🎲 Random 12-byte IV generated per encryption via `RAND_bytes` for semantic security
- 🔑 SHA-256 key derivation from passphrase to 32-byte AES key
- ✅ Overwrite original file or save to a new path
- 🧹 One-click Clear to reset all fields
- 💬 Descriptive status and error messages for every operation
- 📝 Timestamped logging to both `stderr` and `file_encryption.log`
- 🪟 Windows / MinGW-compatible GTK4 UI with `GtkFileChooserNative`
- 🔧 Modular 8-file architecture — each module has a focused single responsibility

---

## System Architecture

The application is organised into four clean layers:

```
┌─────────────────────────────────────┐
│         UI Layer  (GTK4)            │  Window, buttons, labels, dialogs
├─────────────────────────────────────┤
│      Controller Layer               │  GTK signal callbacks, state transitions
├─────────────────────────────────────┤
│       Service Layer                 │  crypto_dispatch / file_handler
├─────────────────────────────────────┤
│       Crypto Layer                  │  caesar_cipher, xor_cipher,
│                                     │  aes_gcm_encrypt, aes_gcm_decrypt
└─────────────────────────────────────┘
           ↓
    File I/O  (fopen / fread / fwrite / ftruncate)
           ↓
    Encrypted / Decrypted Output File
```

### Key Structs

| Struct | Responsibility |
|--------|----------------|
| `AppState` | Owns all runtime state (paths, flags, action label). Replaces all global variables. |
| `BrowseContext` | Groups widget pointers needed by file-browse callbacks. |
| `ActionContext` | Groups widget pointers needed by the encrypt/decrypt action callback. |
| `EncryptContext` | Groups widget pointers needed by key and algorithm callbacks. |
| `ClearContext` | Groups widget pointers needed by the Clear button callback. |

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
Industry-standard symmetric authenticated encryption. Uses a 256-bit key derived from the user's passphrase via SHA-256. GCM mode provides both confidentiality and integrity — a wrong key or tampered file is detected and rejected before any output is written.

- **Key derivation:** SHA-256 hash of passphrase → 32-byte key
- **IV:** 12 bytes, randomly generated per encryption via `RAND_bytes`
- **Authentication tag:** 16 bytes (GCM tag), written immediately after the IV in the file header
- **Output file layout:** `[12B IV][16B GCM tag][ciphertext]`

---

## What Changed — Full Refactoring Log

This section documents every change made in this updated version of the project.

---

### 🔴 Security & Bug Fixes — Critical Changes

#### 1. AES Decryption Bug Fixed — Garbage Bytes at End of Decrypted Files

This is the most critical fix in this update.

| | Before | After |
|-|--------|-------|
| **Symptom** | Decrypted files contained extra/stale bytes at the end, appearing as garbage text | Decrypted file is truncated to the exact plaintext size |
| **Root cause** | `EVP_DecryptUpdate()` withholds the last 16-byte plaintext block until `EVP_DecryptFinal_ex()` — if the output file was previously larger, those stale bytes remained on disk | Total plaintext bytes written are tracked; `ftruncate()` is called after `EVP_DecryptFinal_ex()` to remove all stale bytes |
| **Fix** | None | `ftruncate(fileno(output_file), total_written)` after successful decryption |

**Why `EVP_DecryptUpdate` withholds the last block:** OpenSSL delays releasing the final 16 bytes so it can verify the GCM authentication tag *before* exposing any potentially tampered plaintext. Without calling `EVP_DecryptFinal_ex`, those bytes are never flushed — and skipping `ftruncate` after overwriting an older file leaves stale content at the tail.

---

#### 2. AES Mode Upgraded from ECB to GCM

| | Before | After |
|-|--------|-------|
| **API** | Low-level `AES_encrypt()` / `AES_set_encrypt_key()` | EVP high-level API (`EVP_EncryptInit_ex`, `EVP_EncryptUpdate`, `EVP_EncryptFinal_ex`) |
| **Mode** | ECB — identical plaintext blocks produce identical ciphertext blocks | GCM — randomised, authenticated stream cipher |
| **IV** | None | 12-byte CSPRNG IV prepended to every output file |
| **Integrity** | None — tampered files decrypt silently to garbage | 128-bit GCM tag detects any modification before writing output |
| **Header** | `<openssl/aes.h>` | `<openssl/evp.h>` |

ECB mode is considered cryptographically broken for file encryption because patterns in the plaintext are visible in the ciphertext. GCM resolves this and adds integrity checking in a single step.

---

#### 3. Random IV Added per Encryption

**Before:** No IV was used — the same key always produced the same ciphertext for the same file.

**After:** `RAND_bytes(iv, 12)` generates a cryptographically secure 12-byte IV for every encryption call. The IV is written to the first 12 bytes of the output file and read back at the start of decryption. This ensures semantic security — encrypting the same file twice with the same passphrase always produces completely different ciphertext.

---

#### 4. GCM Authentication Tag for Integrity Verification

**Before:** No integrity check. A corrupted or maliciously modified encrypted file would decrypt silently, possibly producing usable but incorrect data.

**After:** The 16-byte GCM tag is written to bytes 12–27 of every encrypted file (immediately after the IV). During decryption, `EVP_DecryptFinal_ex` verifies the tag. If verification fails (wrong password, wrong algorithm, or tampered file), decryption is aborted and the user sees:

```
Authentication failed – wrong key or tampered data
```

---

### 🟠 Architecture — Full Modular Refactor

#### 5. Single 700-Line File Split into 8 Focused Modules

**Before:** The entire application lived in a single `file_encryptor.c` file (~700 lines), mixing UI construction, file I/O, key validation, and all cipher implementations in one place.

**After:** The code is split into 8 source files, each with its own header, following strict single-responsibility separation:

| Module | Responsibility |
|--------|----------------|
| `main.c` | Application entry point — logger init, GTK app setup, signal connection |
| `logger.c` | Timestamped INFO/WARN/ERROR logging to stderr and log file |
| `file_handler.c` | Safe file open/close helpers (`file_open_read`, `file_open_write`, `file_close_safe`) |
| `caesar_cipher.c` | Caesar cipher encrypt and decrypt |
| `xor_cipher.c` | XOR cipher encrypt and decrypt |
| `aes_gcm.c` | AES-256-GCM encrypt and decrypt via OpenSSL EVP |
| `crypto_dispatcher.c` | Routes encrypt/decrypt calls to the correct algorithm; returns typed `CryptoResult` |
| `ui.c` | Full GTK4 UI construction and all signal callbacks |

---

#### 6. All Global Variables Eliminated

**Before:** Six global variables at file scope:
```c
FILE *fr, *fw;
char *path1, *path2, *Label;
bool fo, fs, ft;
```
These caused hidden coupling between functions, made testing impossible, and created race-condition risks.

**After:** All state lives in the `AppState` struct, allocated once in `ui_activate()` and passed through typed callback context structs. No global variables remain anywhere in the codebase.

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

#### 7. Typed Callback Context Structs Replace Raw Widget Casts

**Before:** Callbacks received untyped `GtkWidget **` arrays (e.g. `widgets[6]`, `widgets1[6]`, `widgets2[3]`, `widgets3[10]`) — error-prone and unreadable magic indices.

**After:** Four dedicated context structs (`BrowseContext`, `ActionContext`, `EncryptContext`, `ClearContext`) hold only the widget pointers each callback actually needs, accessed by meaningful field names rather than array indices.

---

#### 8. `crypto_dispatcher.c` — Centralised Algorithm Routing

**Before:** Algorithm selection via `if/else` chains was scattered across the single monolithic callback.

**After:** `crypto_dispatch()` in `crypto_dispatcher.c` is the single point of routing. It validates all parameters, delegates to the correct cipher module, and returns a typed `CryptoResult` enum. Human-readable messages are produced by `crypto_result_message()`:

```c
CRYPTO_OK               → "Operation completed successfully."
CRYPTO_ERR_INVALID_KEY  → "Invalid key – please check the key constraints."
CRYPTO_ERR_AUTH_FAILED  → "Authentication failed – wrong key or tampered file."
CRYPTO_ERR_IO           → "I/O error during operation."
CRYPTO_ERR_UNKNOWN_ALGO → "Unknown algorithm selected."
CRYPTO_ERR_NULL_PARAM   → "Internal error: NULL parameter."
```

---

### 🟡 New Feature — Timestamped Logging System

#### 9. Logging Module Added (`logger.c` / `logger.h`)

**Before:** No logging. Silent failures were the only outcome of most error conditions.

**After:** A dedicated `logger.c` module provides levelled, timestamped logging to both `stderr` and `file_encryption.log` in the working directory. The `LOG_INFO`, `LOG_WARN`, and `LOG_ERROR` macros automatically capture the source file name and line number at the call site.

```
[2025-06-01 14:23:01] [INFO ] (src/aes_gcm.c:67)  aes_gcm_derive_key: key derived via SHA-256
[2025-06-01 14:23:01] [INFO ] (src/aes_gcm.c:130) aes_gcm_encrypt: completed successfully
[2025-06-01 14:23:05] [ERROR] (src/aes_gcm.c:198) aes_gcm_decrypt: authentication FAILED – wrong key or tampered data
```

The log file is opened in **append mode**, so previous sessions are preserved across runs.

---

### 🟢 Code Quality & Platform — Style, Fixes & Compatibility

#### 10. Windows / MinGW Compatibility Fixed in `ui.c`

**Before:** Earlier versions attempted to use `<gio/gio.h>`, `g_file_read()`, `G_IS_FILE_DESCRIPTOR_BASED`, `dup()`, and `fdopen()` to work around the XDG Desktop Portal on Linux — none of which exist or are needed on Windows.

**After:** The UI compiles cleanly on MinGW32/MinGW64. On Windows, `GtkFileChooserNative` uses the native Win32 common file dialog and `g_file_get_path()` always returns a plain `C:\path\to\file` string that `fopen()` handles directly.

---

#### 11. Three GTK4 UI Bugs Fixed

| Bug | Before | After |
|-----|--------|-------|
| **Files opened at browse time** | `fopen` was called immediately when the user selected a file in the chooser dialog, breaking all file types | Path is stored only; the file is opened inside `on_run_clicked()` when the operation actually runs |
| **`GtkFileChooserNative` memory leak** | The native dialog object was never `g_object_unref`'d after use | `g_object_unref` is called at the top of every response handler |
| **Ready flags reset too early** | Ready flags were reset before the operation ran, causing race conditions and incorrect UI states | Flags are only reset after a successful run or an explicit Clear action |

---

#### 12. Naming Conventions — `snake_case` Throughout

All identifiers now follow consistent `snake_case` with descriptive, module-prefixed names. Examples:

| Old name | New name |
|----------|----------|
| `fo`, `fs`, `ft` | `input_ready`, `output_ready`, `action_ready` |
| `path1`, `path2` | `input_path`, `output_path` |
| `Label` | `action_label` |
| `do_encrypt_decrypt` | `crypto_dispatch` |
| `get_clear` | `on_clear` |
| `widgets[6]` | `BrowseContext`, `ActionContext`, etc. |

---

#### 13. Streaming I/O Buffer Size Defined as a Constant

**Before:** AES functions processed one 16-byte cipher block at a time — extremely slow for large files.

**After:** `IO_BUFFER_SIZE 4096` processes 4 KB per EVP call. Because GCM is a stream-friendly mode, there are no padding concerns and performance on large files is significantly improved.

---

## Folder Structure

```
Updated_Project/
├── Makefile
├── README.md
├── file_encryption.log
├── include/
│   ├── common.h              ← Shared types, constants, AppState struct
│   ├── logger.h              ← Logging interface (INFO/WARN/ERROR macros)
│   ├── file_handler.h        ← Safe file open/close helpers
│   ├── caesar_cipher.h       ← Caesar cipher API
│   ├── xor_cipher.h          ← XOR cipher API
│   ├── aes_gcm.h             ← AES-256-GCM API
│   ├── crypto_dispatcher.h   ← Algorithm selector/dispatcher
│   └── ui.h                  ← GTK4 UI interface
└── src/
    ├── main.c                ← Entry point: logger init, GTK app setup
    ├── logger.c              ← Timestamped logger to stderr + log file
    ├── file_handler.c        ← file_open_read / write / close_safe
    ├── caesar_cipher.c       ← Encrypt & decrypt via character shift
    ├── xor_cipher.c          ← Symmetric XOR stream cipher
    ├── aes_gcm.c             ← AES-256-GCM via OpenSSL EVP API
    ├── crypto_dispatcher.c   ← Routes calls to the correct algorithm
    └── ui.c                  ← Full GTK4 UI and signal callbacks
```

---

## Dependencies

| Dependency | Purpose | Install |
|------------|---------|---------|
| GTK 4 | GUI framework | `sudo apt install libgtk-4-dev` |
| OpenSSL ≥ 1.1 | AES-256-GCM, `RAND_bytes`, SHA-256 key derivation | `sudo apt install libssl-dev` |
| GCC | C compiler | `sudo apt install gcc` |
| Make | Build automation | `sudo apt install make` |

---

## Building & Running

### Using Make (recommended)

```bash
# Build
make

# Build and launch immediately
make run

# Clean build artefacts
make clean
```

### Using GCC directly

```bash
gcc src/*.c -o file_encryptor \
    $(pkg-config --cflags --libs gtk4) \
    -lssl -lcrypto \
    -Iinclude \
    -Wall -Wextra -O2
```

### Windows (MinGW64)

```bash
gcc src/*.c -o file_encryptor.exe \
    $(pkg-config --cflags --libs gtk4) \
    -lssl -lcrypto \
    -Iinclude \
    -Wall -Wextra -O2
```

---

## Usage Guide

1. **Select Input File** — Click *Browse…* next to "File" and choose the file you want to encrypt or decrypt.
2. **Select Output File** — Click *Browse…* next to "Output Folder" and choose where the result will be saved. To overwrite the original, select the same file — the *Overwrite original* checkbox will tick automatically.
3. **Choose Algorithm** — Select one from the dropdown: *Caesar Cipher*, *XOR Cipher*, or *AES-256-GCM*.
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
│  12 bytes  IV        │  Random IV — unique per encryption (RAND_bytes)
├──────────────────────┤
│  16 bytes  GCM Tag   │  Authentication tag — detects tampering or wrong key
├──────────────────────┤
│  N  bytes  Cipher-   │  AES-256-GCM encrypted file content
│            text      │
└──────────────────────┘
```

### Key Derivation

```
passphrase ──→ SHA-256 ──→ 256-bit AES key
```

### Threat Model Coverage

| Threat | Mitigation |
|--------|-----------|
| Ciphertext-only analysis | AES-256-GCM (IND-CCA2 secure) |
| Identical-plaintext pattern leakage | ECB replaced with GCM + unique random IV per encryption |
| File tampering / corruption | GCM authentication tag verified before writing any output |
| IV reuse | Randomly generated 12-byte IV per encryption via `RAND_bytes` |
| Stale bytes in decrypted output | `ftruncate()` to exact plaintext size after `EVP_DecryptFinal_ex` |

---

## Design Patterns

| Pattern | Where Applied |
|---------|--------------|
| **Strategy Pattern** | Each algorithm (`caesar_cipher_encrypt`, `xor_cipher_encrypt`, `aes_gcm_encrypt`) is an interchangeable function selected at runtime by `crypto_dispatch`. |
| **Factory Pattern** | `crypto_dispatch()` in `crypto_dispatcher.c` acts as a factory that delegates to the correct algorithm module based on the algorithm string. |
| **Singleton Pattern** | `AppState` is created once in `ui_activate()` and shared across all callbacks via typed context structs. |
| **Observer Pattern** | GTK's signal system (`g_signal_connect`) implements the observer pattern — UI events notify callbacks without tight coupling between widgets. |

---

## SOLID Principles

| Principle | Implementation |
|-----------|---------------|
| **Single Responsibility** | Each of the 8 source files has exactly one responsibility. `aes_gcm.c` only handles AES. `logger.c` only handles logging. `crypto_dispatcher.c` only handles routing. |
| **Open / Closed** | Adding a new cipher requires only a new `.c`/`.h` module and one new branch in `crypto_dispatch()` — no existing code needs to be modified. |
| **Liskov Substitution** | All cipher functions share compatible `(FILE *input, FILE *output, ...)` signatures and can be substituted for one another in the dispatch layer. |
| **Interface Segregation** | The crypto layer has no knowledge of GTK. The UI layer has no knowledge of OpenSSL. `logger.h` is the only cross-cutting include. |
| **Dependency Inversion** | `crypto_dispatcher.c` depends on the abstract concept of "a function that transforms bytes between two file handles", not on any specific algorithm implementation. |

---

## Known Limitations

1. Key derivation uses a single SHA-256 pass — a future improvement would adopt PBKDF2-HMAC-SHA256 with a random salt and 100,000 iterations to resist brute-force attacks on weak passphrases.
2. Caesar and XOR ciphers are not cryptographically secure and are included for educational demonstration only.
3. No progress bar for large file operations.
4. No automated test suite yet.
5. Cross-platform support is primarily Linux and Windows (MinGW); macOS requires minor build adjustments.

---

## Future Improvements

- [ ] Upgrade key derivation from SHA-256 to PBKDF2-HMAC-SHA256 with random salt
- [ ] Add RSA public-key encryption for key exchange
- [ ] Implement a progress bar for large file encryption
- [ ] Add file drag-and-drop support
- [ ] Add unit tests with a C testing framework (e.g., CUnit, cmocka)
- [ ] Multi-threading for non-blocking UI during heavy encryption
- [ ] Cloud storage integration (upload encrypted output)
- [ ] CMake build system for broader cross-platform support
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
3. Commit with descriptive messages: `git commit -m "feat: add modular file split"`
4. Push and open a Pull Request against `develop`
5. Ensure the code compiles cleanly with `-Wall -Wextra` before submitting

---

*Advanced File Encryption System — Advanced Programming Techniques · 2026*
