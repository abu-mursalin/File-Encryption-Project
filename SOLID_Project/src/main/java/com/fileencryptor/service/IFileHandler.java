package com.fileencryptor.service;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * IFileHandler.java
 * -----------------
 * Abstraction over all file I/O needed by the crypto service.
 *
 * SOLID – Dependency Inversion:
 *   CryptoService depends on this interface, not on java.io.File or
 *   concrete stream implementations, making it testable and replaceable.
 *
 * SOLID – Interface Segregation:
 *   Only the minimal operations required by the crypto layer are exposed.
 */
public interface IFileHandler {

    /**
     * Opens the given file for reading as a binary stream.
     *
     * @param file source file
     * @return     a new {@link InputStream} positioned at byte 0
     * @throws IOException if the file cannot be opened
     */
    InputStream openRead(File file) throws IOException;

    /**
     * Opens the given file for writing as a binary stream.
     * Creates the file if it does not exist; truncates if it does.
     *
     * @param file destination file
     * @return     a new {@link OutputStream}
     * @throws IOException if the file cannot be opened or created
     */
    OutputStream openWrite(File file) throws IOException;

    /**
     * Returns {@code true} if the file exists and is a regular file.
     *
     * @param file the file to check
     * @return     {@code true} when the file is a readable regular file
     */
    boolean exists(File file);

    /**
     * Returns the size of the file in bytes, or -1 if not determinable.
     *
     * @param file the file to measure
     * @return     file length in bytes
     */
    long size(File file);
}
