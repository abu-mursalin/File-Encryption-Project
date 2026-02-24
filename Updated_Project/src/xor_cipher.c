/*============================================================
 * File    : xor_cipher.c
 * Purpose : XOR stream-cipher.  XOR is its own inverse,
 *           so encrypt and decrypt share the same core loop.
 *============================================================*/

#include "xor_cipher.h"
#include "common.h"
#include "logger.h"

#include <stdio.h>

/* ── Static helpers ───────────────────────────────────── */

static bool process_stream(FILE *input_file,
                            FILE *output_file,
                            unsigned char key_byte)
{
    int byte;

    while ((byte = fgetc(input_file)) != EOF) {
        unsigned char out = (unsigned char)byte ^ key_byte;

        if (fputc(out, output_file) == EOF) {
            LOG_ERROR("xor: write error");
            return false;
        }
    }

    if (ferror(input_file)) {
        LOG_ERROR("xor: read error");
        return false;
    }

    return true;
}

/* ── Public API ───────────────────────────────────────── */

bool xor_key_is_valid(int key)
{
    return (key >= XOR_KEY_MIN && key <= XOR_KEY_MAX);
}

bool xor_cipher_encrypt(FILE *input_file, FILE *output_file, int key)
{
    if (input_file == NULL || output_file == NULL) {
        LOG_ERROR("xor_encrypt: NULL file pointer");
        return false;
    }
    if (!xor_key_is_valid(key)) {
        LOG_ERROR("xor_encrypt: key %d out of range [%d, %d]",
                  key, XOR_KEY_MIN, XOR_KEY_MAX);
        return false;
    }

    LOG_INFO("XOR encrypt started (key=0x%02X)", (unsigned)key);
    bool ok = process_stream(input_file, output_file, (unsigned char)key);
    LOG_INFO("XOR encrypt %s", ok ? "completed" : "FAILED");
    return ok;
}

bool xor_cipher_decrypt(FILE *input_file, FILE *output_file, int key)
{
    if (input_file == NULL || output_file == NULL) {
        LOG_ERROR("xor_decrypt: NULL file pointer");
        return false;
    }
    if (!xor_key_is_valid(key)) {
        LOG_ERROR("xor_decrypt: key %d out of range [%d, %d]",
                  key, XOR_KEY_MIN, XOR_KEY_MAX);
        return false;
    }

    /* XOR decryption is identical to encryption */
    LOG_INFO("XOR decrypt started (key=0x%02X)", (unsigned)key);
    bool ok = process_stream(input_file, output_file, (unsigned char)key);
    LOG_INFO("XOR decrypt %s", ok ? "completed" : "FAILED");
    return ok;
}
