/*============================================================
 * File    : caesar_cipher.h
 * Purpose : Caesar cipher encryption and decryption.
 *           Works on arbitrary binary files; only alphabetic
 *           bytes are shifted – all others pass through
 *           unchanged.
 *============================================================*/

#ifndef CAESAR_CIPHER_H
#define CAESAR_CIPHER_H

#include <stdio.h>
#include <stdbool.h>

/*
 * caesar_cipher_encrypt
 *   Reads bytes from input_file, shifts alphabetic chars
 *   forward by key positions (mod 26), writes to output_file.
 *
 *   key      : shift value (CAESAR_KEY_MIN … CAESAR_KEY_MAX)
 *   Returns  : true on success, false on I/O error.
 */
bool caesar_cipher_encrypt(FILE *input_file, FILE *output_file, int key);

/*
 * caesar_cipher_decrypt
 *   Reverse of caesar_cipher_encrypt.
 *
 *   key      : same key used during encryption.
 *   Returns  : true on success, false on I/O error.
 */
bool caesar_cipher_decrypt(FILE *input_file, FILE *output_file, int key);

/*
 * caesar_key_is_valid
 *   Returns true if key is within [CAESAR_KEY_MIN, CAESAR_KEY_MAX].
 */
bool caesar_key_is_valid(int key);

#endif /* CAESAR_CIPHER_H */
