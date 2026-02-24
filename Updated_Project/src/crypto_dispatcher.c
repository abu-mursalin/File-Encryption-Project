/*============================================================
 * File    : crypto_dispatcher.c
 * Purpose : Routes encrypt / decrypt calls to the correct
 *           algorithm module based on the algorithm string.
 *============================================================*/

#include "crypto_dispatcher.h"
#include "common.h"
#include "logger.h"
#include "caesar_cipher.h"
#include "xor_cipher.h"
#include "aes_gcm.h"

#include <stdlib.h>
#include <string.h>

/* ── Private helpers ──────────────────────────────────── */

static bool is_encrypt_action(const char *action)
{
    return (action != NULL && strcmp(action, "Encrypt") == 0);
}

static bool is_decrypt_action(const char *action)
{
    return (action != NULL && strcmp(action, "Decrypt") == 0);
}

/* ── Public API ───────────────────────────────────────── */

const char *crypto_result_message(CryptoResult result)
{
    switch (result) {
        case CRYPTO_OK:               return "Operation completed successfully.";
        case CRYPTO_ERR_INVALID_KEY:  return "Invalid key – please check the key constraints.";
        case CRYPTO_ERR_UNKNOWN_ALGO: return "Unknown algorithm selected.";
        case CRYPTO_ERR_IO:           return "I/O error during operation.";
        case CRYPTO_ERR_AUTH_FAILED:  return "Authentication failed – wrong key or tampered file.";
        case CRYPTO_ERR_NULL_PARAM:   return "Internal error: NULL parameter.";
        default:                      return "Unknown error.";
    }
}

CryptoResult crypto_dispatch(const char *algorithm,
                             const char *action,
                             const char *passphrase,
                             FILE       *input_file,
                             FILE       *output_file)
{
    /* ── Validate parameters ──────────────────────────── */
    if (algorithm == NULL || action == NULL ||
        passphrase == NULL || input_file == NULL || output_file == NULL)
    {
        LOG_ERROR("crypto_dispatch: one or more NULL parameters");
        return CRYPTO_ERR_NULL_PARAM;
    }

    if (!is_encrypt_action(action) && !is_decrypt_action(action)) {
        LOG_ERROR("crypto_dispatch: unknown action '%s'", action);
        return CRYPTO_ERR_NULL_PARAM;
    }

    bool encrypt = is_encrypt_action(action);
    bool ok      = false;

    LOG_INFO("crypto_dispatch: algorithm='%s'  action='%s'",
             algorithm, action);

    /* ── Caesar Cipher ────────────────────────────────── */
    if (strcmp(algorithm, ALGO_CAESAR) == 0) {
        int key = atoi(passphrase);

        if (!caesar_key_is_valid(key)) {
            LOG_WARN("crypto_dispatch: caesar key %d invalid", key);
            return CRYPTO_ERR_INVALID_KEY;
        }

        ok = encrypt
             ? caesar_cipher_encrypt(input_file, output_file, key)
             : caesar_cipher_decrypt(input_file, output_file, key);

        return ok ? CRYPTO_OK : CRYPTO_ERR_IO;
    }

    /* ── XOR Cipher ───────────────────────────────────── */
    if (strcmp(algorithm, ALGO_XOR) == 0) {
        int key = atoi(passphrase);

        if (!xor_key_is_valid(key)) {
            LOG_WARN("crypto_dispatch: xor key %d invalid", key);
            return CRYPTO_ERR_INVALID_KEY;
        }

        ok = encrypt
             ? xor_cipher_encrypt(input_file, output_file, key)
             : xor_cipher_decrypt(input_file, output_file, key);

        return ok ? CRYPTO_OK : CRYPTO_ERR_IO;
    }

    /* ── AES-256-GCM ──────────────────────────────────── */
    if (strcmp(algorithm, ALGO_AES) == 0) {
        if (strlen(passphrase) == 0) {
            LOG_WARN("crypto_dispatch: AES passphrase is empty");
            return CRYPTO_ERR_INVALID_KEY;
        }

        unsigned char aes_key[AES_GCM_KEY_SIZE];
        aes_gcm_derive_key(passphrase, aes_key);

        if (encrypt) {
            ok = aes_gcm_encrypt(input_file, output_file, aes_key);
            return ok ? CRYPTO_OK : CRYPTO_ERR_IO;
        } else {
            ok = aes_gcm_decrypt(input_file, output_file, aes_key);
            return ok ? CRYPTO_OK : CRYPTO_ERR_AUTH_FAILED;
        }
    }

    /* ── Unknown algorithm ────────────────────────────── */
    LOG_ERROR("crypto_dispatch: algorithm '%s' not recognised", algorithm);
    return CRYPTO_ERR_UNKNOWN_ALGO;
}
