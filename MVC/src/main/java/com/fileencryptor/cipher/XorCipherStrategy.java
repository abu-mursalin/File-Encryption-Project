package com.fileencryptor.cipher;

import java.io.InputStream;
import java.io.OutputStream;

/**
 * XorCipherStrategy.java
 * ----------------------
 * Implements the XOR stream cipher.
 *
 * Rules (matching the C implementation):
 *   - Every byte is XOR-ed with the key byte.
 *   - XOR is its own inverse: encrypt == decrypt.
 *   - Key range: 1 – 255 (inclusive).
 *
 * SOLID – Single Responsibility:
 *   Exclusively handles XOR cipher logic; no I/O policy, no UI.
 */
public class XorCipherStrategy implements ICipherStrategy {

    /** Minimum valid XOR key. */
    public static final int KEY_MIN = 1;
    /** Maximum valid XOR key. */
    public static final int KEY_MAX = 255;

    // ── ICipherStrategy ──────────────────────────────────────────────

    @Override
    public void validateKey(String passphrase) {
        int key = parseKey(passphrase);
        if (key < KEY_MIN || key > KEY_MAX) {
            throw new IllegalArgumentException(
                "XOR key must be between " + KEY_MIN + " and " + KEY_MAX +
                ". Got: " + key);
        }
    }

    @Override
    public void encrypt(InputStream input, OutputStream output, String passphrase)
            throws Exception {
        validateKey(passphrase);
        processStream(input, output, (byte) parseKey(passphrase));
    }

    @Override
    public void decrypt(InputStream input, OutputStream output, String passphrase)
            throws Exception {
        // XOR decryption is identical to encryption
        validateKey(passphrase);
        processStream(input, output, (byte) parseKey(passphrase));
    }

    // ── Private helpers ──────────────────────────────────────────────

    /**
     * Streams every byte through XOR with {@code keyByte}.
     * Operates on arbitrary binary content; no byte is special-cased.
     */
    private void processStream(InputStream input, OutputStream output, byte keyByte)
            throws Exception {
        int b;
        while ((b = input.read()) != -1) {
            output.write((b ^ (keyByte & 0xFF)) & 0xFF);
        }
    }

    /** Parses the passphrase to an integer key value. */
    private int parseKey(String passphrase) {
        try {
            return Integer.parseInt(passphrase.trim());
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException(
                "XOR key must be a whole number. Got: '" + passphrase + "'");
        }
    }
}
