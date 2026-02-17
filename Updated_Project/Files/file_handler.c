/* ──────────────────────────────────────────────────────────────────────────
 * file_handler.c
 * Implementation of the File Handling module (Single Responsibility).
 * ──────────────────────────────────────────────────────────────────────── */

#include "file_handler.h"
#include "logger.h"

#include <string.h>
#include <sys/stat.h>

/* ── Public API ─────────────────────────────────────────────────────────── */

bool file_exists(const char *path) {
    if (!path) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

FILE *file_open_read(const char *path) {
    if (!path) {
        LOG_ERROR("file_open_read: NULL path supplied.");
        return NULL;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        LOG_ERROR("file_open_read: cannot open '%s' for reading.", path);
    }
    return f;
}

FILE *file_open_write(const char *path) {
    if (!path) {
        LOG_ERROR("file_open_write: NULL path supplied.");
        return NULL;
    }
    FILE *f = fopen(path, "wb");
    if (!f) {
        LOG_ERROR("file_open_write: cannot open '%s' for writing.", path);
    }
    return f;
}

FILE *file_open_overwrite(const char *path) {
    if (!path) {
        LOG_ERROR("file_open_overwrite: NULL path supplied.");
        return NULL;
    }
    FILE *f = fopen(path, "rb+");
    if (!f) {
        LOG_ERROR("file_open_overwrite: cannot open '%s' for overwrite.", path);
    }
    return f;
}

void file_close_pair(FILE *fr, FILE *fw) {
    if (fr) fclose(fr);
    if (fw) fclose(fw);
}

long file_get_size(const char *path) {
    struct stat st;
    if (!path || stat(path, &st) != 0) {
        LOG_ERROR("file_get_size: cannot stat '%s'.", path ? path : "(null)");
        return -1L;
    }
    return (long)st.st_size;
}

bool paths_are_same(const char *path_a, const char *path_b) {
    if (!path_a || !path_b) return false;
    return strcmp(path_a, path_b) == 0;
}
