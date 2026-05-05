package com.fileencryptor.service;

import com.fileencryptor.cipher.CipherStrategyFactory;
import com.fileencryptor.cipher.ICipherStrategy;
import com.fileencryptor.model.CryptoRequest;
import com.fileencryptor.model.CryptoResult;

import javax.crypto.AEADBadTagException;
import java.io.*;
import java.nio.file.Files;

/**
 * CryptoService.java
 * ------------------
 * Orchestrates a single encrypt/decrypt operation:
 *   1. Validates inputs.
 *   2. Looks up the correct cipher strategy via the factory.
 *   3. Detects same-file (in-place) vs. two-file operation.
 *   4. For in-place: reads file to memory, ciphers in memory, writes back.
 *   5. For two-file: pipes directly from input stream to output stream.
 *   6. Maps exceptions to {@link CryptoResult} codes.
 *   7. Ensures streams are always closed.
 *
 * This is the application's primary business-logic class.
 *
 * SOLID – Single Responsibility:
 *   Orchestration only; cipher logic lives in strategy classes,
 *   file access lives in IFileHandler, logging in ILogger.
 *
 * SOLID – Dependency Inversion:
 *   Depends on the IFileHandler and ILogger interfaces, not
 *   on concrete implementations – making this class fully testable
 *   with mock objects.
 *
 * SOLID – Open/Closed:
 *   New algorithms are added via the factory; this class never changes.
 */
public class CryptoService {

    private final IFileHandler fileHandler;
    private final ILogger      logger;

    /**
     * Constructs a CryptoService with injected dependencies.
     *
     * @param fileHandler abstraction over file I/O
     * @param logger      abstraction over logging
     */
    public CryptoService(IFileHandler fileHandler, ILogger logger) {
        this.fileHandler = fileHandler;
        this.logger      = logger;
    }

    /**
     * Executes the encrypt or decrypt operation described by {@code request}.
     *
     * @param request fully populated {@link CryptoRequest}
     * @return        a {@link CryptoResult} indicating success or failure
     */
    public CryptoResult execute(CryptoRequest request) {

        // ── 1. Null-guard ────────────────────────────────────────────
        if (request == null ||
            request.getInputFile()  == null ||
            request.getOutputFile() == null ||
            request.getAlgorithm()  == null ||
            request.getPassphrase() == null)
        {
            logger.error("CryptoService.execute: null parameter in request");
            return CryptoResult.ERR_NULL_PARAM;
        }

        // ── 2. Verify input file exists ──────────────────────────────
        if (!fileHandler.exists(request.getInputFile())) {
            logger.error("Input file not found: " + request.getInputFile().getAbsolutePath());
            return CryptoResult.ERR_IO;
        }

        // ── 3. Obtain the cipher strategy ────────────────────────────
        ICipherStrategy strategy;
        try {
            strategy = CipherStrategyFactory.create(request.getAlgorithm());
        } catch (IllegalArgumentException e) {
            logger.error("Unknown algorithm: " + request.getAlgorithm());
            return CryptoResult.ERR_UNKNOWN_ALGO;
        }

        // ── 4. Validate the key before opening any files ─────────────
        try {
            strategy.validateKey(request.getPassphrase());
        } catch (IllegalArgumentException e) {
            logger.warn("Key validation failed: " + e.getMessage());
            return CryptoResult.ERR_INVALID_KEY;
        }

        // ── 5. Detect same-file (in-place) operation ─────────────────
        boolean inPlace = request.getInputFile().getAbsolutePath()
                .equals(request.getOutputFile().getAbsolutePath());

        logger.info(String.format("Starting %s with %s  [%s → %s]%s",
                request.isEncrypt() ? "encryption" : "decryption",
                request.getAlgorithm().getDisplayName(),
                request.getInputFile().getName(),
                request.getOutputFile().getName(),
                inPlace ? " (in-place)" : ""));

        try {
            if (inPlace) {
                return executeInPlace(strategy, request);
            } else {
                return executeToFile(strategy, request);
            }

        } catch (AEADBadTagException e) {
            logger.error("AES-GCM authentication FAILED: " + e.getMessage());
            return CryptoResult.ERR_AUTH_FAILED;

        } catch (IllegalArgumentException e) {
            logger.warn("Invalid key: " + e.getMessage());
            return CryptoResult.ERR_INVALID_KEY;

        } catch (Exception e) {
            logger.error("I/O or crypto error: " + e.getMessage());
            return CryptoResult.ERR_IO;
        }
    }

    // ── Private helpers ──────────────────────────────────────────────

    /**
     * Normal two-file operation: input and output are different files.
     * Opens both streams simultaneously and pipes data through the cipher.
     */
    private CryptoResult executeToFile(ICipherStrategy strategy,
                                       CryptoRequest request) throws Exception {
        InputStream  in  = null;
        OutputStream out = null;
        try {
            in  = fileHandler.openRead (request.getInputFile());
            out = fileHandler.openWrite(request.getOutputFile());

            if (request.isEncrypt()) {
                strategy.encrypt(in, out, request.getPassphrase());
            } else {
                strategy.decrypt(in, out, request.getPassphrase());
            }

            logger.info("Operation completed successfully.");
            return CryptoResult.OK;

        } finally {
            closeQuietly(in,  "input stream");
            closeQuietly(out, "output stream");
        }
    }

    /**
     * In-place operation: input and output are the SAME file.
     *
     * WHY the naive approach fails in Java:
     *   Opening a FileOutputStream on an existing file immediately
     *   truncates it to zero bytes — before a single byte is read.
     *   So reading gives nothing, and the file is destroyed.
     *   The C code avoided this by using fopen(path, "rb+") which
     *   opens without truncating.
     *
     * FIX — read-all-then-write-back strategy:
     *   1. Read the entire input file into a byte[] in memory.
     *   2. Run the cipher from a ByteArrayInputStream → ByteArrayOutputStream.
     *      (No file is open at this point — no truncation risk.)
     *   3. Write the result back to the file using Files.write(), which
     *      atomically replaces the file content.
     *
     * This is safe for typical file sizes (documents, images, text).
     * For very large files (>500 MB) consider a temp-file strategy instead.
     */
    private CryptoResult executeInPlace(ICipherStrategy strategy,
                                        CryptoRequest request) throws Exception {

        // Step 1 — Read entire file into memory (file is fully closed after this)
        byte[] inputBytes = Files.readAllBytes(request.getInputFile().toPath());
        logger.info("In-place: read " + inputBytes.length + " bytes into memory.");

        // Step 2 — Run cipher entirely in memory (no file involved)
        ByteArrayInputStream  memIn  = new ByteArrayInputStream(inputBytes);
        ByteArrayOutputStream memOut = new ByteArrayOutputStream(inputBytes.length);

        if (request.isEncrypt()) {
            strategy.encrypt(memIn, memOut, request.getPassphrase());
        } else {
            strategy.decrypt(memIn, memOut, request.getPassphrase());
        }

        // Step 3 — Write result back, replacing file contents atomically
        byte[] outputBytes = memOut.toByteArray();
        Files.write(request.getOutputFile().toPath(), outputBytes);
        logger.info("In-place: wrote " + outputBytes.length + " bytes back to file.");

        logger.info("In-place operation completed successfully.");
        return CryptoResult.OK;
    }

    /** Closes a stream and logs a warning if it throws. */
    private void closeQuietly(AutoCloseable stream, String label) {
        if (stream != null) {
            try { stream.close(); }
            catch (Exception e) {
                logger.warn("Failed to close " + label + ": " + e.getMessage());
            }
        }
    }
}
