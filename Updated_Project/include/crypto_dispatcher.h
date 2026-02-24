/*============================================================
 * File    : crypto_dispatcher.h
 * Purpose : High-level dispatcher that selects the correct
 *           algorithm and calls encrypt / decrypt based on
 *           the algorithm name string and action string.
 *============================================================*/

#ifndef CRYPTO_DISPATCHER_H
#define CRYPTO_DISPATCHER_H

#include <stdio.h>
#include <stdbool.h>

/*
 * CryptoResult – returned by the dispatcher to indicate
 * the outcome of an encrypt / decrypt operation.
 */
typedef enum {
    CRYPTO_OK                = 0,
    CRYPTO_ERR_INVALID_KEY   = 1,
    CRYPTO_ERR_UNKNOWN_ALGO  = 2,
    CRYPTO_ERR_IO            = 3,
    CRYPTO_ERR_AUTH_FAILED   = 4,  /* AES-GCM tag mismatch */
    CRYPTO_ERR_NULL_PARAM    = 5
} CryptoResult;

/*
 * crypto_dispatch
 *   Selects algorithm and direction, runs the operation.
 *
 *   algorithm  : one of ALGO_CAESAR, ALGO_XOR, ALGO_AES
 *   action     : "Encrypt" or "Decrypt"
 *   passphrase : user key / password string
 *   input_file : already-opened input FILE*
 *   output_file: already-opened output FILE*
 *
 *   Returns    : CryptoResult code.
 */
CryptoResult crypto_dispatch(const char *algorithm,
                             const char *action,
                             const char *passphrase,
                             FILE       *input_file,
                             FILE       *output_file);

/*
 * crypto_result_message
 *   Human-readable string for a CryptoResult code.
 */
const char *crypto_result_message(CryptoResult result);

#endif /* CRYPTO_DISPATCHER_H */
