package com.fileencryptor.cipher;

import java.io.InputStream;
import java.io.OutputStream;

/**
 * ICipherStrategy.java
 * --------------------
 * Strategy interface for all cipher implementations.
 *
 * SOLID – Open/Closed:
 *   The dispatcher is closed to modification but open for extension:
 *   add a new cipher by implementing this interface without touching
 *   any existing class.
 *
 * SOLID – Dependency Inversion:
 *   High-level modules (CryptoService, CryptoDispatcher) depend on
 *   this abstraction, not on concrete Caesar/XOR/AES classes.
 *
 * SOLID – Interface Segregation:
 *   Two narrow methods – encrypt and decrypt – rather than one fat
 *   process(direction, ...) method.
 */
public interface ICipherStrategy {

    /**
     * Encrypts bytes from {@code input} and writes ciphertext to {@code output}.
     *
     * @param input      source stream (read-only, position at byte 0)
     * @param output     destination stream (write-only)
     * @param passphrase raw key string supplied by the user
     * @throws Exception on any I/O or cryptographic error
     */
    void encrypt(InputStream input, OutputStream output, String passphrase)
            throws Exception;

    /**
     * Decrypts bytes from {@code input} and writes plaintext to {@code output}.
     *
     * @param input      source stream
     * @param output     destination stream
     * @param passphrase raw key string supplied by the user
     * @throws Exception on any I/O, cryptographic, or authentication error
     */
    void decrypt(InputStream input, OutputStream output, String passphrase)
            throws Exception;

    /**
     * Validates the passphrase before the operation starts.
     * Implementations should throw an {@link IllegalArgumentException}
     * with a descriptive message when the key is out of range.
     *
     * @param passphrase the key/passphrase to check
     */
    void validateKey(String passphrase);
}
