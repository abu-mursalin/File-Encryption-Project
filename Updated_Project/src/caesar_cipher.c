/*============================================================
 * File    : caesar_cipher.c
 * Purpose : Caesar cipher encryption and decryption.
 *           Only ASCII alphabetic characters are shifted;
 *           all other bytes pass through unchanged, making
 *           the cipher safe for arbitrary binary files.
 *============================================================*/

#include "caesar_cipher.h"
#include "common.h"
#include "logger.h"

#include <ctype.h>
#include <stdio.h>

/* ── Static helpers ───────────────────────────────────── */

/*
 * shift_char – applies a Caesar shift to a single character.
 *   base  : 'A' for upper-case, 'a' for lower-case.
 *   shift : positive (encrypt) or negative (decrypt) offset.
 */
static char shift_char(char ch, int base, int shift)
{
    return (char)(((ch - base + shift + 26) % 26) + base);
}

/*
 * process_stream – core loop shared by encrypt and decrypt.
 */
static bool process_stream(FILE *input_file,
                            FILE *output_file,
                            int   shift)
{
    int byte;

    while ((byte = fgetc(input_file)) != EOF) {
        char ch = (char)byte;

        if (isupper((unsigned char)ch))
            ch = shift_char(ch, 'A', shift);
        else if (islower((unsigned char)ch))
            ch = shift_char(ch, 'a', shift);

        if (fputc((unsigned char)ch, output_file) == EOF) {
            LOG_ERROR("caesar: write error");
            return false;
        }
    }

    if (ferror(input_file)) {
        LOG_ERROR("caesar: read error");
        return false;
    }

    return true;
}

/* ── Public API ───────────────────────────────────────── */

bool caesar_key_is_valid(int key)
{
    return (key >= CAESAR_KEY_MIN && key <= CAESAR_KEY_MAX);
}

bool caesar_cipher_encrypt(FILE *input_file, FILE *output_file, int key)
{
    if (input_file == NULL || output_file == NULL) {
        LOG_ERROR("caesar_encrypt: NULL file pointer");
        return false;
    }
    if (!caesar_key_is_valid(key)) {
        LOG_ERROR("caesar_encrypt: key %d out of range [%d, %d]",
                  key, CAESAR_KEY_MIN, CAESAR_KEY_MAX);
        return false;
    }

    LOG_INFO("Caesar encrypt started  (key=%d)", key);
    bool ok = process_stream(input_file, output_file, key);
    LOG_INFO("Caesar encrypt %s", ok ? "completed" : "FAILED");
    return ok;
}

bool caesar_cipher_decrypt(FILE *input_file, FILE *output_file, int key)
{
    if (input_file == NULL || output_file == NULL) {
        LOG_ERROR("caesar_decrypt: NULL file pointer");
        return false;
    }
    if (!caesar_key_is_valid(key)) {
        LOG_ERROR("caesar_decrypt: key %d out of range [%d, %d]",
                  key, CAESAR_KEY_MIN, CAESAR_KEY_MAX);
        return false;
    }

    LOG_INFO("Caesar decrypt started  (key=%d)", key);
    /* Decryption is a negative shift: shift by (26 - key) */
    bool ok = process_stream(input_file, output_file, 26 - key);
    LOG_INFO("Caesar decrypt %s", ok ? "completed" : "FAILED");
    return ok;
}
