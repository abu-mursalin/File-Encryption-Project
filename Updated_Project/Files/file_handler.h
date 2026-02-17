#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stdio.h>
#include <stdbool.h>

/* ──────────────────────────────────────────────────────────────────────────
 * file_handler.h
 * Interface for the File Handling module.
 *
 * Keeping file-I/O helpers separate from crypto logic satisfies:
 *   • Single Responsibility Principle
 *   • Interface Segregation Principle
 * ──────────────────────────────────────────────────────────────────────── */

/**
 * file_exists
 * Returns true if the file at @path can be opened for reading.
 */
bool file_exists(const char *path);

/**
 * file_open_read / file_open_write / file_open_overwrite
 *
 * Thin wrappers around fopen() that log errors internally.
 * Callers receive NULL on failure and should treat it as an error.
 *
 *  file_open_read       – "rb"  – read-only binary
 *  file_open_write      – "wb"  – create / truncate binary
 *  file_open_overwrite  – "rb+" – read+write existing binary (for in-place)
 */
FILE *file_open_read      (const char *path);
FILE *file_open_write     (const char *path);
FILE *file_open_overwrite (const char *path);

/**
 * file_close_pair
 * Closes both file handles, tolerating NULL pointers.
 */
void file_close_pair(FILE *fr, FILE *fw);

/**
 * file_get_size
 * Returns the byte size of the file at @path, or -1 on error.
 */
long file_get_size(const char *path);

/**
 * paths_are_same
 * Returns true when @path_a and @path_b resolve to the same file.
 * Uses strcmp() – sufficient for paths already obtained from GTK.
 */
bool paths_are_same(const char *path_a, const char *path_b);

#endif /* FILE_HANDLER_H */
