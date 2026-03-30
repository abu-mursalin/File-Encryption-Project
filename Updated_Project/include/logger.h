/*============================================================
 * File    : logger.h
 * Purpose : Lightweight logging interface.
 *           Supports INFO, WARN, and ERROR levels.
 *           Log lines are timestamped and written to stderr
 *           (and optionally to a log file).
 *============================================================*/

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

/* Log level constants */
typedef enum {
    LOG_LEVEL_INFO  = 0,
    LOG_LEVEL_WARN  = 1,
    LOG_LEVEL_ERROR = 2
} LogLevel;

/* Minimum level that will be printed (change to filter noise) */
#define LOG_MIN_LEVEL  LOG_LEVEL_INFO

/*
 * logger_init  – optional: open a log file.
 *                Pass NULL to log to stderr only.
 */
void logger_init(const char *log_file_path);

/*
 * logger_close – flush and close the log file (if open).
 */
void logger_close(void);

/*
 * LOG_INFO / LOG_WARN / LOG_ERROR – convenience macros
 * that capture __FILE__ and __LINE__ automatically.
 */
#define LOG_INFO(fmt, ...)  \
    logger_write(LOG_LEVEL_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...)  \
    logger_write(LOG_LEVEL_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    logger_write(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/* Internal implementation – prefer the macros above */
void logger_write(LogLevel level,
                  const char *source_file,
                  int         line,
                  const char *fmt, ...);

#endif /* LOGGER_H */
