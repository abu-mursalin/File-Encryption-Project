package com.fileencryptor.service;

import java.io.*;

/**
 * FileHandlerImpl.java
 * --------------------
 * Default {@link IFileHandler} implementation that delegates to
 * standard {@link java.io} classes with buffering for performance.
 *
 * SOLID – Single Responsibility:
 *   Responsible solely for opening/closing physical files.
 *   Buffering strategy (8 KB) is encapsulated here only.
 */
public class FileHandlerImpl implements IFileHandler {

    /** I/O buffer size – 8 KB balances memory usage vs. syscall overhead. */
    private static final int BUFFER_SIZE = 8192;

    @Override
    public InputStream openRead(File file) throws IOException {
        // Wrap in BufferedInputStream for efficient byte-at-a-time reads
        return new BufferedInputStream(new FileInputStream(file), BUFFER_SIZE);
    }

    @Override
    public OutputStream openWrite(File file) throws IOException {
        // Wrap in BufferedOutputStream to coalesce small writes
        return new BufferedOutputStream(new FileOutputStream(file), BUFFER_SIZE);
    }

    @Override
    public boolean exists(File file) {
        return file != null && file.exists() && file.isFile();
    }

    @Override
    public long size(File file) {
        return (file == null) ? -1L : file.length();
    }
}
