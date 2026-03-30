/*============================================================
 * File    : aes_gcm.h
 * Purpose : AES-256-GCM authenticated encryption /
 *           decryption using OpenSSL EVP API.
 *
 * On-disk format (written by aes_gcm_encrypt):
 *   [ IV  – 12 bytes ]
 *   [ TAG – 16 bytes ]
 *   [ CIPHERTEXT – variable ]
 *
 * The IV is generated with a cryptographically-secure RNG
 * (RAND_bytes) for every encryption operation, so the same
 * plaintext + key will produce different ciphertexts each
 * time.  The TAG provides authenticated integrity – any
 * tampering is detected during decryption.
 *============================================================*/

#ifndef AES_GCM_H
#define AES_GCM_H

#include <stdio.h>
#include <stdbool.h>
#include "common.h"

/*
 * aes_gcm_derive_key
 *   Derives a 256-bit key from an arbitrary-length passphrase
 *   using SHA-256.
 *
 *   passphrase : null-terminated user-supplied string.
 *   out_key    : caller-supplied buffer of at least
 *                AES_GCM_KEY_SIZE bytes.
 */
void aes_gcm_derive_key(const char    *passphrase,
                        unsigned char  out_key[AES_GCM_KEY_SIZE]);

/*
 * aes_gcm_encrypt
 *   Encrypts input_file → output_file using AES-256-GCM.
 *   A random 12-byte IV is generated, prepended to the
 *   output together with the 16-byte authentication tag.
 *
 *   key     : 32-byte derived key (from aes_gcm_derive_key).
 *   Returns : true on success, false on any error.
 */
bool aes_gcm_encrypt(FILE                *input_file,
                     FILE                *output_file,
                     const unsigned char  key[AES_GCM_KEY_SIZE]);

/*
 * aes_gcm_decrypt
 *   Decrypts output of aes_gcm_encrypt.
 *   Reads IV and TAG from the file header, verifies the tag
 *   before writing plaintext.  Returns false if authentication
 *   fails (tampered ciphertext or wrong key).
 *
 *   key     : 32-byte derived key.
 *   Returns : true on success, false on error / auth failure.
 */
bool aes_gcm_decrypt(FILE                *input_file,
                     FILE                *output_file,
                     const unsigned char  key[AES_GCM_KEY_SIZE]);

#endif /* AES_GCM_H */
