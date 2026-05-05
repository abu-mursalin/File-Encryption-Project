package com.fileencryptor.cipher;

import java.io.InputStream;
import java.io.OutputStream;

/**
 * CaesarCipherStrategy.java
 * -------------------------
 * Implements the Caesar cipher for both encrypt and decrypt.
 *
 * Rules (matching the C implementation):
 *   - Only ASCII alphabetic bytes are shifted; all others pass through.
 *   - Key range: 1 – 25 (inclusive).
 *   - Decrypt uses shift = (26 - key) – i.e. the inverse rotation.
 *
 * SOLID – Single Responsibility:
 *   Only responsible for Caesar cipher logic; no file access, no UI.
 */
public class CaesarCipherStrategy implements ICipherStrategy {

    /** Minimum valid shift key. */
    public static final int KEY_MIN = 1;
    /** Maximum valid shift key. */
    public static final int KEY_MAX = 25;

    // ── ICipherStrategy ──────────────────────────────────────────────

    @Override
    public void validateKey(String passphrase) {
        int key = parseKey(passphrase);
        if (key < KEY_MIN || key > KEY_MAX) {
            throw new IllegalArgumentException(
                "Caesar key must be between " + KEY_MIN + " and " + KEY_MAX +
                ". Got: " + key);
        }
    }

    @Override
    public void encrypt(InputStream input, OutputStream output, String passphrase)
            throws Exception {
        validateKey(passphrase);
        processStream(input, output, parseKey(passphrase));
    }

    @Override
    public void decrypt(InputStream input, OutputStream output, String passphrase)
            throws Exception {
        validateKey(passphrase);
        // Decryption is the inverse: shift by (26 - key)
        processStream(input, output, 26 - parseKey(passphrase));
    }

    // ── Private helpers ──────────────────────────────────────────────

    /**
     * Core streaming loop.  Reads every byte; shifts alphabetic chars
     * by {@code shift} positions (mod 26); passes all other bytes unchanged.
     */
    private void processStream(InputStream input, OutputStream output, int shift)
            throws Exception {
        int b;
        while ((b = input.read()) != -1) {
            char ch = (char) b;
            if (Character.isUpperCase(ch)) {
                ch = shiftChar(ch, 'A', shift);
            } else if (Character.isLowerCase(ch)) {
                ch = shiftChar(ch, 'a', shift);
            }
            output.write((byte) ch);
        }
    }

    /**
     * Applies a Caesar rotation to a single alphabetic character.
     *
     * @param ch    character to shift
     * @param base  'A' for upper-case, 'a' for lower-case
     * @param shift rotation amount (may be positive or negative)
     * @return      shifted character, guaranteed within the same case block
     */
    private char shiftChar(char ch, char base, int shift) {
        return (char) (((ch - base + shift + 26) % 26) + base);
    }

    /** Parses the passphrase string to an integer key. */
    private int parseKey(String passphrase) {
        try {
            return Integer.parseInt(passphrase.trim());
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException(
                "Caesar key must be a whole number. Got: '" + passphrase + "'");
        }
    }
}
