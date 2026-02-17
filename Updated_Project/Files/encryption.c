/* ──────────────────────────────────────────────────────────────────────────
 * encryption.c
 * Encryption Engine – implements Caesar, XOR, and AES-256-CBC ciphers.
 *
 * Improvements over original (per Final Report §5 & §6):
 *   [FIX-1]  AES uses EVP API in CBC mode instead of low-level ECB.
 *   [FIX-2]  Random IV generated per operation and prepended to ciphertext.
 *   [FIX-3]  PBKDF2-HMAC-SHA256 replaces raw SHA-256 for key derivation.
 *   [FIX-4]  HMAC-SHA256 appended to ciphertext for integrity verification.
 *   [FIX-5]  Key/IV buffers wiped from memory (OPENSSL_cleanse) after use.
 *   [FIX-6]  All file handles managed locally – no global FILE pointers.
 *   [FIX-7]  Structured return codes instead of silent failures.
 *   [FIX-8]  Buffered streaming (8 KB blocks) for large-file performance.
 *
 * Design patterns applied:
 *   • Strategy Pattern  – each algorithm is an independent function.
 *   • Factory dispatch  – callers select the strategy via Algorithm enum.
 * ──────────────────────────────────────────────────────────────────────── */

#include "encryption.h"
#include "file_handler.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

/* Streaming buffer size for large-file support. */
#define STREAM_BUFFER_SIZE  (8 * 1024)   /* 8 KB                         */

/* ── Status message table ───────────────────────────────────────────────── */

const char *encrypt_status_message(EncryptStatus status) {
    switch (status) {
        case ENCRYPT_OK:            return "Operation completed successfully.";
        case ENCRYPT_ERR_FILE:      return "File error: could not open, read, or write the file.";
        case ENCRYPT_ERR_KEY:       return "Key error: key value is out of the allowed range.";
        case ENCRYPT_ERR_MEMORY:    return "Memory error: failed to allocate required buffer.";
        case ENCRYPT_ERR_CRYPTO:    return "Cryptographic error: OpenSSL operation failed.";
        case ENCRYPT_ERR_INTEGRITY: return "Integrity error: HMAC verification failed – file may be corrupt or tampered.";
        case ENCRYPT_ERR_PADDING:   return "Padding error: invalid PKCS#7 padding – wrong password or corrupt file.";
        default:                    return "Unknown error.";
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * Caesar Cipher  (Strategy 1)
 * ══════════════════════════════════════════════════════════════════════════ */

EncryptStatus caesar_cipher_encrypt(const char *src_path,
                                    const char *dst_path,
                                    int         key) {
    if (key < CAESAR_KEY_MIN || key > CAESAR_KEY_MAX) {
        LOG_ERROR("Caesar encrypt: key %d out of range [%d, %d].",
                  key, CAESAR_KEY_MIN, CAESAR_KEY_MAX);
        return ENCRYPT_ERR_KEY;
    }

    FILE *fr = file_open_read(src_path);
    FILE *fw = paths_are_same(src_path, dst_path)
             ? file_open_overwrite(dst_path)
             : file_open_write(dst_path);

    if (!fr || !fw) {
        file_close_pair(fr, fw);
        return ENCRYPT_ERR_FILE;
    }

    LOG_INFO("Caesar encrypt started: '%s' → '%s', key=%d.", src_path, dst_path, key);

    int c;
    while ((c = fgetc(fr)) != EOF) {
        unsigned char ch = (unsigned char)c;
        if (isupper(ch)) ch = (unsigned char)(((ch + key - 'A') % 26) + 'A');
        else if (islower(ch)) ch = (unsigned char)(((ch + key - 'a') % 26) + 'a');
        if (fputc(ch, fw) == EOF) {
            LOG_ERROR("Caesar encrypt: write error.");
            file_close_pair(fr, fw);
            return ENCRYPT_ERR_FILE;
        }
    }

    file_close_pair(fr, fw);
    LOG_INFO("Caesar encrypt completed.");
    return ENCRYPT_OK;
}

EncryptStatus caesar_cipher_decrypt(const char *src_path,
                                    const char *dst_path,
                                    int         key) {
    if (key < CAESAR_KEY_MIN || key > CAESAR_KEY_MAX) {
        LOG_ERROR("Caesar decrypt: key %d out of range [%d, %d].",
                  key, CAESAR_KEY_MIN, CAESAR_KEY_MAX);
        return ENCRYPT_ERR_KEY;
    }

    FILE *fr = file_open_read(src_path);
    FILE *fw = paths_are_same(src_path, dst_path)
             ? file_open_overwrite(dst_path)
             : file_open_write(dst_path);

    if (!fr || !fw) {
        file_close_pair(fr, fw);
        return ENCRYPT_ERR_FILE;
    }

    LOG_INFO("Caesar decrypt started: '%s' → '%s', key=%d.", src_path, dst_path, key);

    int c;
    while ((c = fgetc(fr)) != EOF) {
        unsigned char ch = (unsigned char)c;
        if (isupper(ch)) ch = (unsigned char)(((ch - key - 'A' + 26) % 26) + 'A');
        else if (islower(ch)) ch = (unsigned char)(((ch - key - 'a' + 26) % 26) + 'a');
        if (fputc(ch, fw) == EOF) {
            LOG_ERROR("Caesar decrypt: write error.");
            file_close_pair(fr, fw);
            return ENCRYPT_ERR_FILE;
        }
    }

    file_close_pair(fr, fw);
    LOG_INFO("Caesar decrypt completed.");
    return ENCRYPT_OK;
}

/* ══════════════════════════════════════════════════════════════════════════
 * XOR Cipher  (Strategy 2)
 * XOR is its own inverse, so encrypt and decrypt share one implementation.
 * ══════════════════════════════════════════════════════════════════════════ */

static EncryptStatus xor_cipher_run(const char *src_path,
                                    const char *dst_path,
                                    int         key,
                                    const char *op_label) {
    if (key < XOR_KEY_MIN || key > XOR_KEY_MAX) {
        LOG_ERROR("XOR %s: key %d out of range [%d, %d].",
                  op_label, key, XOR_KEY_MIN, XOR_KEY_MAX);
        return ENCRYPT_ERR_KEY;
    }

    FILE *fr = file_open_read(src_path);
    FILE *fw = paths_are_same(src_path, dst_path)
             ? file_open_overwrite(dst_path)
             : file_open_write(dst_path);

    if (!fr || !fw) {
        file_close_pair(fr, fw);
        return ENCRYPT_ERR_FILE;
    }

    LOG_INFO("XOR %s started: '%s' → '%s', key=%d.", op_label, src_path, dst_path, key);

    unsigned char buffer[STREAM_BUFFER_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fr)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            buffer[i] ^= (unsigned char)key;
        }
        if (fwrite(buffer, 1, bytes_read, fw) != bytes_read) {
            LOG_ERROR("XOR %s: write error.", op_label);
            file_close_pair(fr, fw);
            return ENCRYPT_ERR_FILE;
        }
    }

    file_close_pair(fr, fw);
    LOG_INFO("XOR %s completed.", op_label);
    return ENCRYPT_OK;
}

EncryptStatus xor_cipher_encrypt(const char *src_path,
                                 const char *dst_path,
                                 int         key) {
    return xor_cipher_run(src_path, dst_path, key, "encrypt");
}

EncryptStatus xor_cipher_decrypt(const char *src_path,
                                 const char *dst_path,
                                 int         key) {
    return xor_cipher_run(src_path, dst_path, key, "decrypt");
}

/* ══════════════════════════════════════════════════════════════════════════
 * AES-256-CBC  (Strategy 3)
 *
 * Encrypted file layout:
 *   [ salt (16 B) | IV (16 B) | ciphertext (N bytes) | HMAC-SHA256 (32 B) ]
 *
 * Key derivation:
 *   PBKDF2-HMAC-SHA256(password, salt, PBKDF2_ITERS) → 64 bytes
 *     first 32 B = AES encryption key
 *     last  32 B = HMAC key
 * ══════════════════════════════════════════════════════════════════════════ */

/* Derive two 32-byte keys from a password + salt using PBKDF2.            */
static EncryptStatus derive_keys(const char    *password,
                                 const unsigned char *salt,
                                 unsigned char  aes_key[AES_KEY_SIZE],
                                 unsigned char  hmac_key[HMAC_SIZE]) {
    unsigned char key_material[AES_KEY_SIZE + HMAC_SIZE];

    int rc = PKCS5_PBKDF2_HMAC(
        password, (int)strlen(password),
        salt, PBKDF2_SALT_SIZE,
        PBKDF2_ITERS,
        EVP_sha256(),
        (int)sizeof(key_material),
        key_material
    );

    if (rc != 1) {
        LOG_ERROR("derive_keys: PBKDF2 failed.");
        OPENSSL_cleanse(key_material, sizeof(key_material));
        return ENCRYPT_ERR_CRYPTO;
    }

    memcpy(aes_key,  key_material,              AES_KEY_SIZE);
    memcpy(hmac_key, key_material + AES_KEY_SIZE, HMAC_SIZE);
    OPENSSL_cleanse(key_material, sizeof(key_material)); /* [FIX-5] */
    return ENCRYPT_OK;
}

/* ── AES Encrypt ─────────────────────────────────────────────────────────── */

EncryptStatus aes_encrypt(const char *src_path,
                          const char *dst_path,
                          const char *password) {
    if (!password || strlen(password) == 0) {
        LOG_ERROR("aes_encrypt: empty password.");
        return ENCRYPT_ERR_KEY;
    }

    /* [FIX-2] Generate random salt and IV. */
    unsigned char salt[PBKDF2_SALT_SIZE];
    unsigned char iv[AES_IV_SIZE];
    if (RAND_bytes(salt, sizeof(salt)) != 1 ||
        RAND_bytes(iv,   sizeof(iv))   != 1) {
        LOG_ERROR("aes_encrypt: RAND_bytes failed.");
        return ENCRYPT_ERR_CRYPTO;
    }

    /* [FIX-3] Derive AES key + HMAC key via PBKDF2. */
    unsigned char aes_key[AES_KEY_SIZE];
    unsigned char hmac_key[HMAC_SIZE];
    EncryptStatus ks = derive_keys(password, salt, aes_key, hmac_key);
    if (ks != ENCRYPT_OK) return ks;

    /* Open files. */
    FILE *fr = file_open_read(src_path);
    FILE *fw = paths_are_same(src_path, dst_path)
             ? file_open_overwrite(dst_path)
             : file_open_write(dst_path);

    if (!fr || !fw) {
        file_close_pair(fr, fw);
        OPENSSL_cleanse(aes_key, sizeof(aes_key));
        OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
        return ENCRYPT_ERR_FILE;
    }

    LOG_INFO("AES-256-CBC encrypt started: '%s' → '%s'.", src_path, dst_path);

    /* Write salt and IV. */
    fwrite(salt, 1, sizeof(salt), fw);
    fwrite(iv,   1, sizeof(iv),   fw);

    /* [FIX-1] EVP_EncryptInit with CBC mode. */
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        LOG_ERROR("aes_encrypt: EVP_CIPHER_CTX_new failed.");
        file_close_pair(fr, fw);
        OPENSSL_cleanse(aes_key, sizeof(aes_key));
        OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
        return ENCRYPT_ERR_CRYPTO;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, aes_key, iv) != 1) {
        LOG_ERROR("aes_encrypt: EVP_EncryptInit_ex failed.");
        EVP_CIPHER_CTX_free(ctx);
        file_close_pair(fr, fw);
        OPENSSL_cleanse(aes_key, sizeof(aes_key));
        OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
        return ENCRYPT_ERR_CRYPTO;
    }

    /* [FIX-4] HMAC context covers ciphertext (salt+IV already written). */
    HMAC_CTX *hmac_ctx = HMAC_CTX_new();
    HMAC_Init_ex(hmac_ctx, hmac_key, HMAC_SIZE, EVP_sha256(), NULL);
    HMAC_Update(hmac_ctx, salt, sizeof(salt));
    HMAC_Update(hmac_ctx, iv,   sizeof(iv));

    /* [FIX-8] Buffered streaming encryption. */
    unsigned char in_buf[STREAM_BUFFER_SIZE];
    unsigned char out_buf[STREAM_BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH];
    int out_len = 0;
    size_t bytes_read;

    while ((bytes_read = fread(in_buf, 1, sizeof(in_buf), fr)) > 0) {
        if (EVP_EncryptUpdate(ctx, out_buf, &out_len,
                              in_buf, (int)bytes_read) != 1) {
            LOG_ERROR("aes_encrypt: EVP_EncryptUpdate failed.");
            EVP_CIPHER_CTX_free(ctx);
            HMAC_CTX_free(hmac_ctx);
            file_close_pair(fr, fw);
            OPENSSL_cleanse(aes_key, sizeof(aes_key));
            OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
            return ENCRYPT_ERR_CRYPTO;
        }
        fwrite(out_buf, 1, (size_t)out_len, fw);
        HMAC_Update(hmac_ctx, out_buf, (size_t)out_len);
    }

    /* Flush PKCS#7 padding block. */
    if (EVP_EncryptFinal_ex(ctx, out_buf, &out_len) != 1) {
        LOG_ERROR("aes_encrypt: EVP_EncryptFinal_ex failed.");
        EVP_CIPHER_CTX_free(ctx);
        HMAC_CTX_free(hmac_ctx);
        file_close_pair(fr, fw);
        OPENSSL_cleanse(aes_key, sizeof(aes_key));
        OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
        return ENCRYPT_ERR_CRYPTO;
    }
    fwrite(out_buf, 1, (size_t)out_len, fw);
    HMAC_Update(hmac_ctx, out_buf, (size_t)out_len);

    /* Append HMAC. */
    unsigned char hmac_digest[HMAC_SIZE];
    unsigned int  hmac_len = 0;
    HMAC_Final(hmac_ctx, hmac_digest, &hmac_len);
    fwrite(hmac_digest, 1, hmac_len, fw);

    /* [FIX-5] Wipe key material. */
    OPENSSL_cleanse(aes_key,  sizeof(aes_key));
    OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
    OPENSSL_cleanse(iv,       sizeof(iv));

    EVP_CIPHER_CTX_free(ctx);
    HMAC_CTX_free(hmac_ctx);
    file_close_pair(fr, fw);

    LOG_INFO("AES-256-CBC encrypt completed.");
    return ENCRYPT_OK;
}

/* ── AES Decrypt ─────────────────────────────────────────────────────────── */

EncryptStatus aes_decrypt(const char *src_path,
                          const char *dst_path,
                          const char *password) {
    if (!password || strlen(password) == 0) {
        LOG_ERROR("aes_decrypt: empty password.");
        return ENCRYPT_ERR_KEY;
    }

    /* Minimum valid file: salt + IV + 1 ciphertext block + HMAC. */
    long file_size = file_get_size(src_path);
    long min_size  = (long)(PBKDF2_SALT_SIZE + AES_IV_SIZE + 16 + HMAC_SIZE);
    if (file_size < min_size) {
        LOG_ERROR("aes_decrypt: file too small to be valid AES output (%ld B).", file_size);
        return ENCRYPT_ERR_FILE;
    }

    FILE *fr = file_open_read(src_path);
    if (!fr) return ENCRYPT_ERR_FILE;

    /* Read salt and IV. */
    unsigned char salt[PBKDF2_SALT_SIZE];
    unsigned char iv[AES_IV_SIZE];
    if (fread(salt, 1, sizeof(salt), fr) != sizeof(salt) ||
        fread(iv,   1, sizeof(iv),   fr) != sizeof(iv)) {
        LOG_ERROR("aes_decrypt: could not read salt/IV.");
        fclose(fr);
        return ENCRYPT_ERR_FILE;
    }

    /* [FIX-3] Derive matching keys. */
    unsigned char aes_key[AES_KEY_SIZE];
    unsigned char hmac_key[HMAC_SIZE];
    EncryptStatus ks = derive_keys(password, salt, aes_key, hmac_key);
    if (ks != ENCRYPT_OK) {
        fclose(fr);
        return ks;
    }

    LOG_INFO("AES-256-CBC decrypt started: '%s' → '%s'.", src_path, dst_path);

    /*
     * Read the entire remaining ciphertext into a heap buffer so we can
     * split off the trailing HMAC before feeding data to EVP_Decrypt.
     */
    long ciphertext_and_hmac_size = file_size - PBKDF2_SALT_SIZE - AES_IV_SIZE;
    size_t cipher_size = (size_t)(ciphertext_and_hmac_size - HMAC_SIZE);

    unsigned char *ciphertext_buf = malloc((size_t)ciphertext_and_hmac_size);
    if (!ciphertext_buf) {
        LOG_ERROR("aes_decrypt: malloc failed (%ld bytes).", ciphertext_and_hmac_size);
        fclose(fr);
        OPENSSL_cleanse(aes_key, sizeof(aes_key));
        OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
        return ENCRYPT_ERR_MEMORY;
    }

    if (fread(ciphertext_buf, 1, (size_t)ciphertext_and_hmac_size, fr)
            != (size_t)ciphertext_and_hmac_size) {
        LOG_ERROR("aes_decrypt: unexpected end of file.");
        free(ciphertext_buf);
        fclose(fr);
        OPENSSL_cleanse(aes_key, sizeof(aes_key));
        OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
        return ENCRYPT_ERR_FILE;
    }
    fclose(fr);

    /* [FIX-4] Verify HMAC before any decryption. */
    unsigned char stored_hmac[HMAC_SIZE];
    memcpy(stored_hmac, ciphertext_buf + cipher_size, HMAC_SIZE);

    unsigned char computed_hmac[HMAC_SIZE];
    unsigned int  computed_len = 0;
    HMAC_CTX *hmac_ctx = HMAC_CTX_new();
    HMAC_Init_ex(hmac_ctx, hmac_key, HMAC_SIZE, EVP_sha256(), NULL);
    HMAC_Update(hmac_ctx, salt,           sizeof(salt));
    HMAC_Update(hmac_ctx, iv,             sizeof(iv));
    HMAC_Update(hmac_ctx, ciphertext_buf, cipher_size);
    HMAC_Final(hmac_ctx, computed_hmac, &computed_len);
    HMAC_CTX_free(hmac_ctx);

    if (CRYPTO_memcmp(stored_hmac, computed_hmac, HMAC_SIZE) != 0) {
        LOG_ERROR("aes_decrypt: HMAC mismatch – wrong password or corrupt file.");
        free(ciphertext_buf);
        OPENSSL_cleanse(aes_key, sizeof(aes_key));
        OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
        return ENCRYPT_ERR_INTEGRITY;
    }

    /* HMAC OK – proceed with decryption. */
    FILE *fw = paths_are_same(src_path, dst_path)
             ? file_open_overwrite(dst_path)
             : file_open_write(dst_path);
    if (!fw) {
        free(ciphertext_buf);
        OPENSSL_cleanse(aes_key, sizeof(aes_key));
        OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
        return ENCRYPT_ERR_FILE;
    }

    /* [FIX-1] EVP_DecryptInit with CBC mode. */
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx || EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, aes_key, iv) != 1) {
        LOG_ERROR("aes_decrypt: EVP init failed.");
        if (ctx) EVP_CIPHER_CTX_free(ctx);
        fclose(fw);
        free(ciphertext_buf);
        OPENSSL_cleanse(aes_key, sizeof(aes_key));
        OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
        return ENCRYPT_ERR_CRYPTO;
    }

    unsigned char *plaintext_buf = malloc(cipher_size + EVP_MAX_BLOCK_LENGTH);
    if (!plaintext_buf) {
        LOG_ERROR("aes_decrypt: plaintext malloc failed.");
        EVP_CIPHER_CTX_free(ctx);
        fclose(fw);
        free(ciphertext_buf);
        OPENSSL_cleanse(aes_key, sizeof(aes_key));
        OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
        return ENCRYPT_ERR_MEMORY;
    }

    int out_len1 = 0, out_len2 = 0;

    if (EVP_DecryptUpdate(ctx, plaintext_buf, &out_len1,
                          ciphertext_buf, (int)cipher_size) != 1) {
        LOG_ERROR("aes_decrypt: EVP_DecryptUpdate failed.");
        EVP_CIPHER_CTX_free(ctx);
        fclose(fw);
        free(ciphertext_buf);
        free(plaintext_buf);
        OPENSSL_cleanse(aes_key, sizeof(aes_key));
        OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
        return ENCRYPT_ERR_CRYPTO;
    }

    /* [FIX-6/7] EVP_DecryptFinal validates PKCS#7 padding. */
    if (EVP_DecryptFinal_ex(ctx, plaintext_buf + out_len1, &out_len2) != 1) {
        LOG_ERROR("aes_decrypt: EVP_DecryptFinal_ex failed – bad padding.");
        EVP_CIPHER_CTX_free(ctx);
        fclose(fw);
        free(ciphertext_buf);
        OPENSSL_cleanse(plaintext_buf, cipher_size + EVP_MAX_BLOCK_LENGTH);
        free(plaintext_buf);
        OPENSSL_cleanse(aes_key, sizeof(aes_key));
        OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
        return ENCRYPT_ERR_PADDING;
    }

    fwrite(plaintext_buf, 1, (size_t)(out_len1 + out_len2), fw);

    /* [FIX-5] Wipe all sensitive heap and stack buffers. */
    OPENSSL_cleanse(plaintext_buf, cipher_size + EVP_MAX_BLOCK_LENGTH);
    free(plaintext_buf);
    free(ciphertext_buf);
    OPENSSL_cleanse(aes_key,  sizeof(aes_key));
    OPENSSL_cleanse(hmac_key, sizeof(hmac_key));
    OPENSSL_cleanse(iv,       sizeof(iv));

    EVP_CIPHER_CTX_free(ctx);
    fclose(fw);

    LOG_INFO("AES-256-CBC decrypt completed.");
    return ENCRYPT_OK;
}
