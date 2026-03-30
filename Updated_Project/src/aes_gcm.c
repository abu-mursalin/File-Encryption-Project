/*============================================================
 * File    : aes_gcm.c
 * Purpose : AES-256-GCM authenticated encryption using
 *           OpenSSL EVP (high-level) API.
 *
 * On-disk layout produced by aes_gcm_encrypt:
 *   Offset 0                  : IV  (AES_GCM_IV_SIZE  = 12 bytes)
 *   Offset AES_GCM_IV_SIZE    : TAG (AES_GCM_TAG_SIZE = 16 bytes)
 *   Offset AES_GCM_IV_SIZE +
 *          AES_GCM_TAG_SIZE   : Ciphertext (variable length)
 *
 * ── BUG FIX NOTES ─────────────────────────────────────────
 *
 * PROBLEM (extra/garbage bytes at end of decrypted file):
 *
 *   OpenSSL's EVP_DecryptUpdate() intentionally withholds the
 *   last 16 bytes of plaintext during the streaming loop.
 *   It does this so it can verify the GCM authentication tag
 *   BEFORE releasing the final plaintext block.
 *   Those last 16 bytes are only flushed by EVP_DecryptFinal_ex().
 *
 *   If the output file was previously larger than the decrypted
 *   result (e.g. decrypting in-place or overwriting an older
 *   file), the unflushed tail bytes leave stale data in the file
 *   beyond the last written position — appearing as extra
 *   words / garbage at the end.
 *
 * FIX:
 *   Track the total number of plaintext bytes written.
 *   After EVP_DecryptFinal_ex() succeeds, call ftruncate()
 *   to cut the file to exactly that many bytes.
 *   This guarantees no stale content survives regardless of
 *   the output file's prior size.
 *
 *============================================================*/

#include "aes_gcm.h"
#include "logger.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ftruncate() / fileno() – POSIX, available on Linux & macOS */
#include <unistd.h>

/* ── I/O buffer size ──────────────────────────────────────── */
#define IO_BUFFER_SIZE  4096

/* ── Public API ───────────────────────────────────────────── */

void aes_gcm_derive_key(const char    *passphrase,
                        unsigned char  out_key[AES_GCM_KEY_SIZE])
{
    /* SHA-256 produces exactly 32 bytes – perfect for AES-256 */
    SHA256((const unsigned char *)passphrase,
           strlen(passphrase),
           out_key);

    LOG_INFO("aes_gcm_derive_key: key derived via SHA-256");
}

/* ─────────────────────────────────────────────────────────────
 * aes_gcm_encrypt
 * ───────────────────────────────────────────────────────────── */
bool aes_gcm_encrypt(FILE                *input_file,
                     FILE                *output_file,
                     const unsigned char  key[AES_GCM_KEY_SIZE])
{
    if (input_file == NULL || output_file == NULL || key == NULL) {
        LOG_ERROR("aes_gcm_encrypt: NULL parameter");
        return false;
    }

    bool            success = false;
    EVP_CIPHER_CTX *ctx     = NULL;

    /* ── 1. Generate a cryptographically random 12-byte IV ── */
    unsigned char iv[AES_GCM_IV_SIZE];
    if (RAND_bytes(iv, AES_GCM_IV_SIZE) != 1) {
        LOG_ERROR("aes_gcm_encrypt: RAND_bytes failed");
        goto cleanup;
    }

    /* ── 2. Write IV + zero TAG placeholder to file header ── */
    /*
     * We reserve space for the TAG now and seek back to fill
     * it after encryption finishes (the tag is only available
     * after EVP_EncryptFinal_ex).
     */
    unsigned char tag_placeholder[AES_GCM_TAG_SIZE] = {0};
    if (fwrite(iv,              1, AES_GCM_IV_SIZE,  output_file) != AES_GCM_IV_SIZE  ||
        fwrite(tag_placeholder, 1, AES_GCM_TAG_SIZE, output_file) != AES_GCM_TAG_SIZE)
    {
        LOG_ERROR("aes_gcm_encrypt: failed to write IV/TAG header");
        goto cleanup;
    }

    /* ── 3. Initialise EVP cipher context ────────────────── */
    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        LOG_ERROR("aes_gcm_encrypt: EVP_CIPHER_CTX_new failed");
        goto cleanup;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN,
                             AES_GCM_IV_SIZE, NULL) != 1 ||
        EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv) != 1)
    {
        LOG_ERROR("aes_gcm_encrypt: EVP init failed");
        goto cleanup;
    }

    /* ── 4. Encrypt the plaintext in IO_BUFFER_SIZE chunks ── */
    unsigned char in_buf[IO_BUFFER_SIZE];
    unsigned char out_buf[IO_BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH];
    int    out_len   = 0;
    size_t bytes_read;

    while ((bytes_read = fread(in_buf, 1, sizeof(in_buf), input_file)) > 0) {
        if (EVP_EncryptUpdate(ctx, out_buf, &out_len,
                              in_buf, (int)bytes_read) != 1)
        {
            LOG_ERROR("aes_gcm_encrypt: EVP_EncryptUpdate failed");
            goto cleanup;
        }
        if (out_len > 0 &&
            fwrite(out_buf, 1, (size_t)out_len, output_file) != (size_t)out_len)
        {
            LOG_ERROR("aes_gcm_encrypt: write ciphertext chunk failed");
            goto cleanup;
        }
    }

    if (ferror(input_file)) {
        LOG_ERROR("aes_gcm_encrypt: input read error");
        goto cleanup;
    }

    /* ── 5. Finalise – flushes any remaining ciphertext ──── */
    if (EVP_EncryptFinal_ex(ctx, out_buf, &out_len) != 1) {
        LOG_ERROR("aes_gcm_encrypt: EVP_EncryptFinal_ex failed");
        goto cleanup;
    }
    if (out_len > 0 &&
        fwrite(out_buf, 1, (size_t)out_len, output_file) != (size_t)out_len)
    {
        LOG_ERROR("aes_gcm_encrypt: write final ciphertext block failed");
        goto cleanup;
    }

    /* ── 6. Retrieve GCM authentication tag ──────────────── */
    unsigned char tag[AES_GCM_TAG_SIZE];
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG,
                             AES_GCM_TAG_SIZE, tag) != 1)
    {
        LOG_ERROR("aes_gcm_encrypt: GET_TAG failed");
        goto cleanup;
    }

    /* ── 7. Seek back to offset 12 and write the real tag ── */
    if (fseek(output_file, (long)AES_GCM_IV_SIZE, SEEK_SET) != 0 ||
        fwrite(tag, 1, AES_GCM_TAG_SIZE, output_file) != AES_GCM_TAG_SIZE)
    {
        LOG_ERROR("aes_gcm_encrypt: failed to write authentication tag");
        goto cleanup;
    }

    LOG_INFO("aes_gcm_encrypt: completed successfully");
    success = true;

cleanup:
    if (ctx != NULL)
        EVP_CIPHER_CTX_free(ctx);

    return success;
}

/* ─────────────────────────────────────────────────────────────
 * aes_gcm_decrypt
 * ───────────────────────────────────────────────────────────── */
bool aes_gcm_decrypt(FILE                *input_file,
                     FILE                *output_file,
                     const unsigned char  key[AES_GCM_KEY_SIZE])
{
    if (input_file == NULL || output_file == NULL || key == NULL) {
        LOG_ERROR("aes_gcm_decrypt: NULL parameter");
        return false;
    }

    bool            success       = false;
    EVP_CIPHER_CTX *ctx           = NULL;
    long            total_written = 0;   /* ← track exact bytes written */

    /* ── 1. Read IV and TAG from the file header ─────────── */
    unsigned char iv[AES_GCM_IV_SIZE];
    unsigned char tag[AES_GCM_TAG_SIZE];

    if (fread(iv,  1, AES_GCM_IV_SIZE,  input_file) != AES_GCM_IV_SIZE ||
        fread(tag, 1, AES_GCM_TAG_SIZE, input_file) != AES_GCM_TAG_SIZE)
    {
        LOG_ERROR("aes_gcm_decrypt: file too short – missing IV/TAG header");
        goto cleanup;
    }

    /* ── 2. Initialise EVP cipher context ────────────────── */
    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        LOG_ERROR("aes_gcm_decrypt: EVP_CIPHER_CTX_new failed");
        goto cleanup;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN,
                             AES_GCM_IV_SIZE, NULL) != 1 ||
        EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv) != 1)
    {
        LOG_ERROR("aes_gcm_decrypt: EVP init failed");
        goto cleanup;
    }

    /* ── 3. Decrypt ciphertext in chunks ─────────────────── */
    /*
     * NOTE: EVP_DecryptUpdate withholds the final 16-byte block
     * until EVP_DecryptFinal_ex() is called.  That is intentional
     * — OpenSSL verifies the GCM tag against those bytes before
     * releasing them.  Do NOT expect the loop alone to produce
     * all plaintext bytes; the last block arrives in step 5.
     */
    unsigned char in_buf[IO_BUFFER_SIZE];
    unsigned char out_buf[IO_BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH];
    int    out_len = 0;
    size_t bytes_read;

    while ((bytes_read = fread(in_buf, 1, sizeof(in_buf), input_file)) > 0) {
        if (EVP_DecryptUpdate(ctx, out_buf, &out_len,
                              in_buf, (int)bytes_read) != 1)
        {
            LOG_ERROR("aes_gcm_decrypt: EVP_DecryptUpdate failed");
            goto cleanup;
        }
        if (out_len > 0) {
            if (fwrite(out_buf, 1, (size_t)out_len, output_file)
                    != (size_t)out_len)
            {
                LOG_ERROR("aes_gcm_decrypt: write plaintext chunk failed");
                goto cleanup;
            }
            total_written += out_len;  /* ← count every byte written */
        }
    }

    if (ferror(input_file)) {
        LOG_ERROR("aes_gcm_decrypt: input read error");
        goto cleanup;
    }

    /* ── 4. Set the expected tag so OpenSSL can verify it ── */
    /*
     * EVP_CTRL_GCM_SET_TAG must be called BEFORE
     * EVP_DecryptFinal_ex.  Final_ex will return <= 0 if the
     * tag does not match (wrong key, corrupted file, etc.).
     */
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG,
                             AES_GCM_TAG_SIZE, tag) != 1)
    {
        LOG_ERROR("aes_gcm_decrypt: SET_TAG failed");
        goto cleanup;
    }

    /* ── 5. Finalise – releases the withheld last block ──── */
    /*
     * This is where the last 16 bytes of plaintext are written.
     * If this step is skipped or the output is not truncated
     * afterwards, the decrypted file will contain extra/stale
     * bytes at the end.
     */
    int final_ret = EVP_DecryptFinal_ex(ctx, out_buf, &out_len);
    if (final_ret <= 0) {
        LOG_ERROR("aes_gcm_decrypt: authentication FAILED – "
                  "wrong key or tampered data");
        goto cleanup;
    }

    if (out_len > 0) {
        if (fwrite(out_buf, 1, (size_t)out_len, output_file)
                != (size_t)out_len)
        {
            LOG_ERROR("aes_gcm_decrypt: write final plaintext block failed");
            goto cleanup;
        }
        total_written += out_len;  /* ← count the final block too */
    }

    /* ── 6. Truncate the output file to the exact plaintext size
     *
     *  WHY THIS IS NECESSARY:
     *  If the output file previously existed and was larger than
     *  the decrypted result (e.g. decrypting over an older file,
     *  or a same-file overwrite scenario), bytes beyond our last
     *  fwrite() position remain on disk.  fwrite does NOT erase
     *  them.  ftruncate() removes those stale tail bytes so the
     *  file is exactly the right size.
     * ──────────────────────────────────────────────────────── */
    fflush(output_file);
    if (ftruncate(fileno(output_file), (off_t)total_written) != 0) {
        LOG_WARN("aes_gcm_decrypt: ftruncate failed – "
                 "output file may have extra bytes at the end");
        /*
         * Non-fatal: the plaintext content is correct; only the
         * file size may be wrong.  Log and continue.
         */
    }

    LOG_INFO("aes_gcm_decrypt: success – %ld plaintext bytes written",
             total_written);
    success = true;

cleanup:
    if (ctx != NULL)
        EVP_CIPHER_CTX_free(ctx);

    return success;
}
