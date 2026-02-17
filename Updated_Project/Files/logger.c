/* ──────────────────────────────────────────────────────────────────────────
 * logger.c
 * Implementation of the logging module.
 * ──────────────────────────────────────────────────────────────────────── */

#include "logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static FILE *log_file = NULL;

/* ── Internal helpers ───────────────────────────────────────────────────── */

static const char *level_label(LogLevel level) {
    switch (level) {
        case LOG_INFO:  return "INFO ";
        case LOG_WARN:  return "WARN ";
        case LOG_ERROR: return "ERROR";
        default:        return "?????";
    }
}

static void write_timestamp(FILE *dest) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
    fprintf(dest, "[%s] ", buf);
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void log_init(const char *log_path) {
    if (log_path) {
        log_file = fopen(log_path, "a");
        if (!log_file) {
            fprintf(stderr, "[LOGGER] Warning: could not open log file '%s'.\n",
                    log_path);
        }
    }
}

void log_close(void) {
    if (log_file) {
        fflush(log_file);
        fclose(log_file);
        log_file = NULL;
    }
}

void log_message(LogLevel level, const char *fmt, ...) {
    va_list args;

    /* Write to stderr. */
    write_timestamp(stderr);
    fprintf(stderr, "[%s] ", level_label(level));
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

    /* Mirror to log file if open. */
    if (log_file) {
        write_timestamp(log_file);
        fprintf(log_file, "[%s] ", level_label(level));
        va_start(args, fmt);
        vfprintf(log_file, fmt, args);
        va_end(args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
}
