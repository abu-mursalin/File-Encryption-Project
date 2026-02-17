#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stdbool.h>
#include <stddef.h>

/* ──────────────────────────────────────────────────────────────────────────
 * encryption.h
 * Interface for the Encryption Engine module.
 *
 * Principles applied (per SOLID / doc requirements):
 *   • Interface Segregation  – only encryption-related declarations here.
 *   • Open/Closed            – add new algorithms without touching callers.
 *   • Dependency Inversion   – callers depend on this abstraction, not on
 *                              concrete crypto functions.
 * ──────────────────────────────────────────────────────────────────────── */

/* Return codes shared across the entire project. */
typedef enum {
    ENCRYPT_OK              =  0,
    ENCRYPT_ERR_FILE        = -1,   /* File open / read / write failure    */
    ENCRYPT_ERR_KEY         = -2,   /* Invalid or out-of-range key          */
    ENCRYPT_ERR_MEMORY      = -3,   /* malloc / buffer allocation failure   */
    ENCRYPT_ERR_CRYPTO      = -4,   /* OpenSSL internal error               */
    ENCRYPT_ERR_INTEGRITY   = -5,   /* HMAC verification failed             */
    ENCRYPT_ERR_PADDING     = -6,   /* Bad PKCS#7 padding on decrypt        */
} EncryptStatus;

/* Supported algorithm identifiers. */
typedef enum {
    ALGO_CAESAR = 0,
    ALGO_XOR    = 1,
    ALGO_AES    = 2,
} Algorithm;

/* Key bounds. */
#define CAESAR_KEY_MIN   1
#define CAESAR_KEY_MAX  25
#define XOR_KEY_MIN      1
#define XOR_KEY_MAX    255

/* AES / crypto constants. */
#define AES_IV_SIZE      16   /* 128-bit IV for CBC mode                  */
#define AES_KEY_SIZE     32   /* 256-bit AES key                          */
#define HMAC_SIZE        32   /* SHA-256 HMAC digest size                 */
#define PBKDF2_ITERS  10000   /* PBKDF2-HMAC-SHA256 iteration count       */
#define PBKDF2_SALT_SIZE 16   /* Random salt for key derivation           */

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * caesar_cipher_encrypt / caesar_cipher_decrypt
 *
 * Encrypt/decrypt the file at @src_path into @dst_path using Caesar cipher.
 * @key must be in [CAESAR_KEY_MIN, CAESAR_KEY_MAX].
 *
 * Returns ENCRYPT_OK on success, negative EncryptStatus on failure.
 */
EncryptStatus caesar_cipher_encrypt(const char *src_path,
                                    const char *dst_path,
                                    int         key);

EncryptStatus caesar_cipher_decrypt(const char *src_path,
                                    const char *dst_path,
                                    int         key);

/**
 * xor_cipher_encrypt / xor_cipher_decrypt
 *
 * XOR is self-inverse; both functions share identical logic.
 * @key must be in [XOR_KEY_MIN, XOR_KEY_MAX].
 *
 * Returns ENCRYPT_OK on success, negative EncryptStatus on failure.
 */
EncryptStatus xor_cipher_encrypt(const char *src_path,
                                 const char *dst_path,
                                 int         key);

EncryptStatus xor_cipher_decrypt(const char *src_path,
                                 const char *dst_path,
                                 int         key);

/**
 * aes_encrypt / aes_decrypt
 *
 * AES-256-CBC with:
 *   • PBKDF2-HMAC-SHA256 key derivation  (replaces raw SHA-256)
 *   • Random IV prepended to ciphertext  (replaces fixed/no IV)
 *   • HMAC-SHA256 appended for integrity (replaces no verification)
 *   • PKCS#7 padding
 *   • Key memory wiped after use
 *
 * @password  – NUL-terminated passphrase string.
 *
 * Returns ENCRYPT_OK on success, negative EncryptStatus on failure.
 */
EncryptStatus aes_encrypt(const char *src_path,
                          const char *dst_path,
                          const char *password);

EncryptStatus aes_decrypt(const char *src_path,
                          const char *dst_path,
                          const char *password);

/**
 * encrypt_status_message
 *
 * Returns a human-readable string for a given EncryptStatus code.
 * The returned string is statically allocated and must NOT be freed.
 */
const char *encrypt_status_message(EncryptStatus status);

#endif /* ENCRYPTION_H */
