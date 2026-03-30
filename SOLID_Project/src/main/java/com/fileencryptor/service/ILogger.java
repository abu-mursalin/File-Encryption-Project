package com.fileencryptor.service;

/**
 * ILogger.java
 * ------------
 * Minimal logging abstraction for the application.
 *
 * SOLID – Dependency Inversion:
 *   Business logic classes depend on this interface, never on a
 *   specific logging framework (java.util.logging, Log4j, etc.).
 *
 * SOLID – Interface Segregation:
 *   Only three log levels exposed; the application does not need
 *   fine-grained levels like TRACE or FATAL.
 */
public interface ILogger {

    /** Logs an informational message. */
    void info(String message);

    /** Logs a warning that does not prevent operation from completing. */
    void warn(String message);

    /** Logs an error that caused or will cause an operation to fail. */
    void error(String message);
}
