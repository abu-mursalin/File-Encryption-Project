/*============================================================
 * File    : file_handler.c
 * Purpose : Safe file open / close helpers.
 *
 * ── WHY fopen() FAILS FOR JPG/PNG ON WINDOWS (MinGW) ──────
 *
 * g_file_get_path() returns a UTF-8 encoded string, e.g.:
 *   "C:\Users\Ahmad\Pictures\photo.jpg"
 *
 * Windows fopen() does NOT accept UTF-8 strings. It uses the
 * system ANSI codepage (e.g. Windows-1252 or CP1256 for Arabic
 * locales). If the path contains ANY character outside plain
 * ASCII — including accented letters, Arabic, Chinese, or even
 * smart quotes — fopen() silently fails and returns NULL.
 *
 * This is the root cause of "Error opening input file" when
 * trying to encrypt a JPG or PNG.
 *
 * THE FIX:
 *   On Windows (_WIN32), convert the UTF-8 path to a wide
 *   (UTF-16) string using MultiByteToWideChar(), then call
 *   _wfopen() which accepts wide strings and works correctly
 *   with ALL Windows paths regardless of locale or codepage.
 *
 *   On Linux/macOS the filesystem is natively UTF-8, so plain
 *   fopen() is fine and we compile that path instead.
 *
 *============================================================*/

#include "file_handler.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/* ── Windows-specific: UTF-8 path → wide FILE* ─────────── */
#ifdef _WIN32
#include <windows.h>

/*
 * utf8_to_wide  –  Convert a UTF-8 string to a newly allocated
 *                  wchar_t* string.  Caller must free() it.
 *                  Returns NULL on failure.
 */
static wchar_t *utf8_to_wide(const char *utf8)
{
    if (utf8 == NULL) return NULL;

    /* First call: get required buffer size */
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (wide_len <= 0) {
        LOG_ERROR("utf8_to_wide: MultiByteToWideChar size query failed "
                  "(error %lu)", GetLastError());
        return NULL;
    }

    wchar_t *wide = (wchar_t *)malloc((size_t)wide_len * sizeof(wchar_t));
    if (wide == NULL) {
        LOG_ERROR("utf8_to_wide: malloc failed");
        return NULL;
    }

    /* Second call: do the actual conversion */
    if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, wide_len) <= 0) {
        LOG_ERROR("utf8_to_wide: MultiByteToWideChar conversion failed "
                  "(error %lu)", GetLastError());
        free(wide);
        return NULL;
    }

    return wide;
}

/*
 * fopen_utf8  –  Drop-in replacement for fopen() that accepts
 *               UTF-8 paths on Windows by converting to wide
 *               chars and calling _wfopen().
 */
static FILE *fopen_utf8(const char *path, const char *mode)
{
    /* Convert path to wide */
    wchar_t *wide_path = utf8_to_wide(path);
    if (wide_path == NULL)
        return NULL;

    /* Convert mode string to wide (mode is always ASCII so this
     * is trivially safe, but _wfopen requires wchar_t mode)    */
    wchar_t *wide_mode = utf8_to_wide(mode);
    if (wide_mode == NULL) {
        free(wide_path);
        return NULL;
    }

    FILE *fp = _wfopen(wide_path, wide_mode);

    free(wide_path);
    free(wide_mode);

    return fp;
}

#else
/* On Linux / macOS the filesystem is natively UTF-8 */
#define fopen_utf8(path, mode)  fopen((path), (mode))
#endif  /* _WIN32 */

/* ═══════════════════════════════════════════════════════════
 * Public API
 * ═══════════════════════════════════════════════════════════ */

FILE *file_open_read(const char *file_path)
{
    if (file_path == NULL) {
        LOG_ERROR("file_open_read: file_path is NULL");
        return NULL;
    }

    /*
     * Use fopen_utf8 instead of fopen so that paths containing
     * non-ASCII characters (accents, CJK, Arabic, etc.) work
     * correctly on Windows regardless of system codepage.
     */
    FILE *fp = fopen_utf8(file_path, "rb");
    if (fp == NULL) {
        LOG_ERROR("file_open_read: failed to open '%s'", file_path);
    } else {
        LOG_INFO("Opened for reading: %s", file_path);
    }
    return fp;
}

FILE *file_open_write(const char *output_path,
                      const char *input_path,
                      bool        overwrite)
{
    if (output_path == NULL) {
        LOG_ERROR("file_open_write: output_path is NULL");
        return NULL;
    }

    FILE *fp = NULL;

    if (overwrite &&
        input_path != NULL &&
        strcmp(output_path, input_path) == 0)
    {
        fp = fopen_utf8(output_path, "rb+");
        LOG_INFO("Opened for overwrite (rb+): %s", output_path);
    } else {
        fp = fopen_utf8(output_path, "wb");
        LOG_INFO("Opened for writing (wb): %s", output_path);
    }

    if (fp == NULL) {
        LOG_ERROR("file_open_write: failed to open '%s'", output_path);
    }
    return fp;
}

void file_close_safe(FILE **fp)
{
    if (fp != NULL && *fp != NULL) {
        fclose(*fp);
        *fp = NULL;
    }
}

bool file_exists(const char *file_path)
{
    if (file_path == NULL)
        return false;

#ifdef _WIN32
    /*
     * Use GetFileAttributesW with a wide path so that non-ASCII
     * paths are checked correctly on Windows.
     */
    wchar_t *wide = utf8_to_wide(file_path);
    if (wide == NULL) return false;
    DWORD attr = GetFileAttributesW(wide);
    free(wide);
    return (attr != INVALID_FILE_ATTRIBUTES &&
            !(attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(file_path, &st) == 0 && S_ISREG(st.st_mode));
#endif
}

long file_size(FILE *fp)
{
    if (fp == NULL)
        return -1L;

    long original_pos = ftell(fp);
    if (original_pos == -1L)
        return -1L;

    if (fseek(fp, 0L, SEEK_END) != 0)
        return -1L;

    long size = ftell(fp);
    fseek(fp, original_pos, SEEK_SET);
    return size;
}
