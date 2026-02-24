/*============================================================
 * File    : xor_cipher.h
 * Purpose : XOR stream-cipher encryption / decryption.
 *           XOR is symmetric: the same function encrypts
 *           and decrypts.  Provided as two named wrappers
 *           for clarity.
 *============================================================*/

#ifndef XOR_CIPHER_H
#define XOR_CIPHER_H

#include <stdio.h>
#include <stdbool.h>

/*
 * xor_cipher_encrypt
 *   XORs every byte of input_file with key and writes the
 *   result to output_file.
 *
 *   key     : XOR mask (XOR_KEY_MIN … XOR_KEY_MAX)
 *   Returns : true on success, false on I/O error.
 */
bool xor_cipher_encrypt(FILE *input_file, FILE *output_file, int key);

/*
 * xor_cipher_decrypt
 *   Identical to xor_cipher_encrypt (XOR is its own inverse).
 *   Provided as a separate function for API symmetry.
 */
bool xor_cipher_decrypt(FILE *input_file, FILE *output_file, int key);

/*
 * xor_key_is_valid
 *   Returns true if key is within [XOR_KEY_MIN, XOR_KEY_MAX].
 */
bool xor_key_is_valid(int key);

#endif /* XOR_CIPHER_H */
