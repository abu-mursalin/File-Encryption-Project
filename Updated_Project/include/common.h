/*============================================================
 * File    : common.h
 * Purpose : Shared types, constants, and global state used
 *           across all modules of the File Encryption project.
 *============================================================*/

#ifndef COMMON_H
#define COMMON_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── AES-256-GCM constants ─────────────────────────────── */
#define AES_GCM_KEY_SIZE   32   /* 256 bits */
#define AES_GCM_IV_SIZE    12   /* 96-bit random IV  (NIST recommended) */
#define AES_GCM_TAG_SIZE   16   /* 128-bit authentication tag */

/* ── Caesar cipher key bounds ──────────────────────────── */
#define CAESAR_KEY_MIN      1
#define CAESAR_KEY_MAX     25

/* ── XOR cipher key bounds ─────────────────────────────── */
#define XOR_KEY_MIN         1
#define XOR_KEY_MAX       255

/* ── Algorithm name strings (match GTK combo-box entries) ─ */
#define ALGO_CAESAR   "Caesar Cipher"
#define ALGO_XOR      "Xor Cipher"
#define ALGO_AES      "AES-256-GCM"

/* ── Application identifier ────────────────────────────── */
#define APP_ID        "com.example.file_encryption"
#define APP_TITLE     "File Encryptor"
#define APP_WIDTH      700
#define APP_HEIGHT     400

/*
 * AppState – central application state passed between
 * GTK callbacks and crypto/file modules.
 */
typedef struct AppState {
    FILE   *input_file;    /* opened input file handle  */
    FILE   *output_file;   /* opened output file handle */
    char   *input_path;    /* heap-allocated path string */
    char   *output_path;   /* heap-allocated path string */
    char   *action_label;  /* "Encrypt" | "Decrypt" | NULL */
    bool    action_chosen; /* radio button selected       */
    bool    input_ready;   /* input file chosen          */
    bool    output_ready;  /* output file chosen         */
} AppState;

#endif /* COMMON_H */
