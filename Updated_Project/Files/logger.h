#ifndef LOGGER_H
#define LOGGER_H

/* ──────────────────────────────────────────────────────────────────────────
 * logger.h
 * Minimal logging interface (Observer pattern, single-responsibility).
 *
 * Recommended in the Final Report (§ 6.2) and Thesis doc.
 * Writes timestamped entries to stderr and optionally to a log file.
 * ──────────────────────────────────────────────────────────────────────── */

typedef enum {
    LOG_INFO  = 0,
    LOG_WARN  = 1,
    LOG_ERROR = 2,
} LogLevel;

/**
 * log_init
 * Optional: open a persistent log file at @log_path.
 * Pass NULL to log to stderr only.
 */
void log_init(const char *log_path);

/**
 * log_close
 * Flush and close the log file (call at application exit).
 */
void log_close(void);

/**
 * log_message
 * Write a formatted message at @level.
 * Example:  log_message(LOG_INFO, "AES encrypt started: %s", src_path);
 */
void log_message(LogLevel level, const char *fmt, ...);

/* Convenience macros. */
#define LOG_INFO(...)  log_message(LOG_INFO,  __VA_ARGS__)
#define LOG_WARN(...)  log_message(LOG_WARN,  __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_ERROR, __VA_ARGS__)

#endif /* LOGGER_H */
