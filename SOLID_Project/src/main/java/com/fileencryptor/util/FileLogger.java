package com.fileencryptor.util;

import com.fileencryptor.service.ILogger;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

/**
 * FileLogger.java
 * ---------------
 * Concrete {@link ILogger} that writes timestamped, levelled messages
 * to both System.err and an optional log file – mirroring the
 * behaviour of logger.c in the original C project.
 *
 * SOLID – Single Responsibility:
 *   Only responsible for writing log entries. Formatting, timestamping,
 *   and file management are all private concerns of this class.
 *
 * Thread-safety note: all public methods are synchronised so the logger
 * can safely be called from background crypto threads.
 */
public class FileLogger implements ILogger, AutoCloseable {

    // ── Formatting ───────────────────────────────────────────────────
    private static final DateTimeFormatter TIMESTAMP_FMT =
            DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");

    // ── State ────────────────────────────────────────────────────────
    private final PrintWriter fileWriter;  // null if no log file was requested

    /**
     * Creates a logger that writes to System.err only.
     */
    public FileLogger() {
        this.fileWriter = null;
    }

    /**
     * Creates a logger that writes to System.err AND appends to
     * {@code logFilePath}.  Silently falls back to stderr-only if the
     * file cannot be opened.
     *
     * @param logFilePath path to the log file (e.g. "file_encryption.log")
     */
    public FileLogger(String logFilePath) {
        PrintWriter pw = null;
        if (logFilePath != null && !logFilePath.isBlank()) {
            try {
                // true = append mode, matches the C logger_init("a") fopen call
                pw = new PrintWriter(new BufferedWriter(
                        new FileWriter(logFilePath, true)));
            } catch (IOException e) {
                System.err.println("[LOGGER] WARNING: Could not open log file: "
                        + logFilePath);
            }
        }
        this.fileWriter = pw;
    }

    // ── ILogger ──────────────────────────────────────────────────────

    @Override
    public synchronized void info(String message) {
        write("INFO ", message);
    }

    @Override
    public synchronized void warn(String message) {
        write("WARN ", message);
    }

    @Override
    public synchronized void error(String message) {
        write("ERROR", message);
    }

    // ── AutoCloseable ────────────────────────────────────────────────

    /** Flushes and closes the underlying log file, if any. */
    @Override
    public synchronized void close() {
        if (fileWriter != null) {
            fileWriter.flush();
            fileWriter.close();
        }
    }

    // ── Private helpers ──────────────────────────────────────────────

    /**
     * Formats and writes a log line to System.err and the optional file.
     * Format: [yyyy-MM-dd HH:mm:ss] [LEVEL] message
     */
    private void write(String level, String message) {
        String timestamp = LocalDateTime.now().format(TIMESTAMP_FMT);
        String line = String.format("[%s] [%s] %s", timestamp, level, message);

        System.err.println(line);

        if (fileWriter != null) {
            fileWriter.println(line);
            fileWriter.flush();   // flush after each entry, same as C fflush()
        }
    }
}
