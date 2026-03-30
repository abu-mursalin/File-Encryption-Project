/*============================================================
 * File    : logger.c
 * Purpose : Timestamped, levelled logging to stderr and an
 *           optional log file.
 *============================================================*/

#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

/* ── Module-private state ─────────────────────────────── */
static FILE *s_log_file = NULL;

/* ── Helpers ──────────────────────────────────────────── */
static const char *level_to_string(LogLevel level)
{
    switch (level) {
        case LOG_LEVEL_INFO:  return "INFO ";
        case LOG_LEVEL_WARN:  return "WARN ";
        case LOG_LEVEL_ERROR: return "ERROR";
        default:              return "?????";
    }
}

static void write_timestamp(FILE *dest)
{
    time_t now   = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(dest, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
}

/* ── Public API ───────────────────────────────────────── */

void logger_init(const char *log_file_path)
{
    if (log_file_path != NULL) {
        s_log_file = fopen(log_file_path, "a");
        if (s_log_file == NULL) {
            fprintf(stderr, "[LOGGER] WARNING: Could not open log file: %s\n",
                    log_file_path);
        }
    }
}

void logger_close(void)
{
    if (s_log_file != NULL) {
        fclose(s_log_file);
        s_log_file = NULL;
    }
}

void logger_write(LogLevel    level,
                  const char *source_file,
                  int         line,
                  const char *fmt, ...)
{
    if (level < LOG_MIN_LEVEL)
        return;

    /* Format the user message into a local buffer */
    char message_buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message_buf, sizeof(message_buf), fmt, args);
    va_end(args);

    /* Write to stderr */
    write_timestamp(stderr);
    fprintf(stderr, "[%s] (%s:%d) %s\n",
            level_to_string(level), source_file, line, message_buf);
    fflush(stderr);

    /* Optionally mirror to log file */
    if (s_log_file != NULL) {
        write_timestamp(s_log_file);
        fprintf(s_log_file, "[%s] (%s:%d) %s\n",
                level_to_string(level), source_file, line, message_buf);
        fflush(s_log_file);
    }
}
