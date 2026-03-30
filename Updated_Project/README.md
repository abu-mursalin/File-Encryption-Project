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
- [Known Limitations](#known-limitations)
- [Future Improvements](#future-improvements)
- [Development History — Step-by-Step Problem Solving](#-development-history--step-by-step-problem-solving)
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

## 🧾 Development History — Step-by-Step Problem Solving

This section documents every real problem encountered and solved during the development of this project, in the exact order they occurred.

---

### 🔨 Step 1 — Project Upgrade: Monolithic to Modular Architecture

The original project was a single `File_Encryption.c` file (~700 lines) mixing UI, file I/O, key validation, and all cipher implementations together. The task was to completely restructure the project with the following requirements:

- Apply proper `snake_case` naming conventions throughout the entire codebase
- Split the code into 8 focused modules: `main.c`, `logger.c`, `file_handler.c`, `caesar_cipher.c`, `xor_cipher.c`, `aes_gcm.c`, `crypto_dispatcher.c`, `ui.c` — each with its own header file
- Replace the broken AES-ECB implementation with AES-256-GCM using the OpenSSL EVP high-level API
- Generate a cryptographically secure random 12-byte IV via `RAND_bytes()` for every encryption operation
- Prepend the IV and the 16-byte GCM authentication tag to the output file header
- Replace the deprecated `AES_encrypt()` / `AES_decrypt()` calls with `EVP_EncryptInit_ex` / `EVP_DecryptFinal_ex`
- Add a timestamped logging system with INFO, WARN, and ERROR levels writing to both `stderr` and a log file
- Replace all global variables with a single `AppState` struct passed through typed callback context structs
- Replace raw `GtkWidget**` array callbacks with typed context structs (`BrowseContext`, `ActionContext`, `EncryptContext`, `ClearContext`)
- Produce a working `Makefile` and a complete `README.md`

---

### 📦 Step 2 — Missing Header Files After Download

After downloading and extracting the project, several files listed in the folder structure were missing. The `include/` folder was incomplete. The missing header files were identified and provided:

- `caesar_cipher.h`
- `xor_cipher.h`
- `file_handler.h`
- `crypto_dispatcher.h`
- `ui.h`

---

### 🐛 Step 3 — AES-256-GCM Decryption Bug: Extra Words at End of File

**Problem:** AES-256-GCM encryption worked correctly but decryption produced extra garbage bytes at the end of every decrypted file.

**Root cause (two bugs working together):**
1. `EVP_DecryptUpdate()` intentionally withholds the last 16-byte plaintext block during the streaming loop so it can verify the GCM authentication tag before releasing that data. Those bytes only appear after `EVP_DecryptFinal_ex()` is called.
2. If the output file previously existed and was larger than the decrypted result, bytes beyond the last `fwrite()` position were not erased — they remained on disk as stale content.

**Fix:** Track the total number of plaintext bytes written across both the loop and `EVP_DecryptFinal_ex()`. After decryption succeeds, call `ftruncate(fileno(output_file), total_written)` to cut the output file to the exact plaintext size.

---

### 🖼️ Step 4 — Browse Error: "Error Opening File" for JPG and PNG

**Problem:** AES-256-GCM could encrypt text files but browsing an image file (`.jpg`, `.png`) caused an error opening the file.

**Root cause:** The original code called `file_open_read()` and stored the `FILE*` in `AppState` at browse time — inside the `on_response_open()` callback. On many desktop environments (GNOME, KDE), the file chooser dialog or thumbnail service reads the same file descriptor after the dialog closes, moving the read position away from byte 0. By the time Encrypt was clicked, `fread()` was reading from mid-file or EOF, producing zero or partial data.

**Fix:** Store only the file path string inside the browse callback. Open both the input and output files inside `on_run_clicked()`, immediately before calling `crypto_dispatch()`, then close them immediately after. This guarantees the file descriptor is always at position 0 for every operation, regardless of file type.

---

### 🚫 Step 5 — "Input File Not Accessible" for JPG and PNG

**Problem:** After the previous fix, browsing a `.jpg` or `.png` showed "Input file not accessible" and the file was rejected before any encryption could occur.

**Root cause:** A `file_exists()` pre-check using `stat()` was added to the browse callback. On modern GNOME/KDE/Wayland systems, `GtkFileChooserNative` routes through the XDG Desktop Portal. The portal returns a `GFile` whose path points to a FUSE-mounted location like `/run/user/1000/doc/a3f9c2b1/photo.jpg`. If the `xdg-document-portal` FUSE daemon was not running, `stat()` reported the file as non-existent even though it was perfectly accessible. Additionally, `g_file_get_path()` returned `NULL` on some portal-backed `GFile` objects, causing the NULL check to fire immediately.

**Fix:** Remove the `file_exists()` pre-check from the browse callback entirely. Replace `g_file_get_path()` with a two-step fallback: try `g_file_get_path()` first, then fall back to `g_filename_from_uri(g_file_get_uri())` to handle URI-backed portal files. Validate accessibility with a real `fopen("rb")` probe instead of `stat()`.

---

### ❌ Step 6 — "Cannot Read Selected File" Still Showing

**Problem:** After the previous fix, browsing a `.jpg` still showed "Cannot read selected file" even though the file existed and was accessible.

**Root cause:** The `fopen("rb")` probe added in the previous fix was itself failing. On modern GNOME/Wayland, even when `g_filename_from_uri()` succeeded and returned a path, that path pointed to the FUSE-mounted portal location. If the FUSE daemon was not running, `fopen()` on that path failed with `ENOENT`.

**Root fix:** Stop using `fopen()` entirely for the input file. Use GIO's `g_file_read()` instead, which communicates with the portal over D-Bus and bypasses the FUSE filesystem completely. The `GFileInputStream` returned by `g_file_read()` implements `GFileDescriptorBased`, so its underlying file descriptor can be extracted with `g_file_descriptor_based_get_fd()`, duplicated with `dup()`, and wrapped as a standard `FILE*` via `fdopen()` for use by the existing crypto functions without any changes to `aes_gcm.c` or other modules.

---

### ⚙️ Step 7 — MinGW32 Compile Error: `obj/ui.o` Error 1

**Problem:** The previous `ui.c` included `#include <gio/gio.h>` and used `g_file_read()`, `G_IS_FILE_DESCRIPTOR_BASED()`, `g_file_descriptor_based_get_fd()`, `dup()`, and `fdopen()`. Building with `mingw32-make` on Windows failed with:

```
mingw32-make: *** [Makefile:56: obj/ui.o] Error 1
```

**Root cause:** All of those Linux-only GIO portal APIs do not exist in the MinGW GTK4 bundle on Windows. The `<gio/gio.h>` standalone header is not available in the MinGW distribution, and the portal/FUSE problem does not exist on Windows at all.

**Fix:** Remove all Linux-only code from `ui.c`. On Windows, `GtkFileChooserNative` uses the native Win32 `GetOpenFileName` dialog. `g_file_get_path()` always returns a standard `C:\path\to\file` string that `fopen()` handles directly. The simplified Windows-compatible `ui.c` uses only standard GTK4 headers (`<gtk/gtk.h>`) and compiles cleanly with `mingw32-make`.

---

### 🔓 Step 8 — "Error Opening Input File" When Clicking Encrypt

**Problem:** After browse was fixed and the project compiled successfully on Windows, clicking the Encrypt button showed "Error opening input file" in the status label for any image file.

**Root cause:** `g_file_get_path()` returns a **UTF-8 encoded** string on all platforms including Windows. However, Windows `fopen()` uses the **system ANSI codepage** (such as CP1252 for Western Europe or CP1256 for Arabic locales) — not UTF-8. If the file path contained any character outside plain ASCII — including accented characters, Arabic letters, Chinese characters, or any non-Latin script — `fopen()` silently returned `NULL`, which appeared as "Error opening input file".

**Fix:** Replace all `fopen()` calls in `file_handler.c` with a `fopen_utf8()` helper function that on Windows:
1. Converts the UTF-8 path string to a wide (`wchar_t*`) string using `MultiByteToWideChar(CP_UTF8, ...)`
2. Calls `_wfopen()` which accepts wide strings and works correctly with all Windows paths regardless of locale or codepage

On Linux and macOS, the filesystem is natively UTF-8 so `fopen_utf8` compiles as a plain macro that calls `fopen()` directly. No changes to any other module were required.

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
