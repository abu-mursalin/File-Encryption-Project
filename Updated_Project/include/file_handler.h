/*============================================================
 * File    : file_handler.h
 * Purpose : Safe file open / close / read / write helpers
 *           and path-related utilities.
 *============================================================*/

#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stdio.h>
#include <stdbool.h>

/*
 * file_open_read  – opens a file for binary reading.
 * Returns a valid FILE* or NULL on failure.
 */
FILE *file_open_read(const char *file_path);

/*
 * file_open_write – opens a file for binary writing.
 * If overwrite == true and in_path == out_path the file
 * is opened for read+write (rb+).
 */
FILE *file_open_write(const char *output_path,
                      const char *input_path,
                      bool        overwrite);

/*
 * file_close_safe – closes *fp if non-NULL and sets it to NULL.
 */
void file_close_safe(FILE **fp);

/*
 * file_exists – returns true if the path points to a
 * readable regular file.
 */
bool file_exists(const char *file_path);

/*
 * file_size – returns file size in bytes, or -1 on error.
 */
long file_size(FILE *fp);

#endif /* FILE_HANDLER_H */
