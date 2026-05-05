package com.fileencryptor.cipher;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.MessageDigest;
import java.security.SecureRandom;

/**
 * AesGcmCipherStrategy.java
 * -------------------------
 * Implements AES-256-GCM authenticated encryption/decryption
 * using the standard Java Cryptography Architecture (JCA/JCE).
 *
 * On-stream layout produced by encrypt():
 *   [ IV  – 12 bytes (random per operation) ]
 *   [ Ciphertext + GCM authentication tag   ]  ← JCA appends tag after ciphertext
 *
 * NOTE: Java's AES/GCM/NoPadding cipher appends the 16-byte
 * authentication tag automatically at the end of the ciphertext during
 * encryption, and verifies + strips it automatically during decryption.
 * This differs slightly from the C implementation (which stored the tag
 * at offset 12 before the ciphertext), but is the standard JCA pattern.
 *
 * Key derivation: SHA-256(passphrase) → 32-byte AES-256 key,
 * exactly matching the C aes_gcm_derive_key() function.
 *
 * SOLID – Single Responsibility:
 *   Only responsible for AES-GCM cipher logic; passphrase derivation
 *   is a private concern of this class.
 */
public class AesGcmCipherStrategy implements ICipherStrategy {

    // ── AES-GCM constants (match C implementation) ───────────────────
    private static final int IV_SIZE_BYTES  = 12;   // 96-bit IV
    private static final int TAG_SIZE_BITS  = 128;  // 128-bit auth tag
    private static final int KEY_SIZE_BYTES = 32;   // AES-256
    private static final String CIPHER_ALGO = "AES/GCM/NoPadding";

    // ── ICipherStrategy ──────────────────────────────────────────────

    @Override
    public void validateKey(String passphrase) {
        if (passphrase == null || passphrase.isEmpty()) {
            throw new IllegalArgumentException(
                "AES passphrase must not be empty.");
        }
    }

    /**
     * Encrypts using AES-256-GCM.
     *
     * Steps:
     *   1. Generate a cryptographically secure random 12-byte IV.
     *   2. Write the IV as the first 12 bytes of output.
     *   3. Derive the 256-bit key from the passphrase via SHA-256.
     *   4. Encrypt the full input; JCA appends the 16-byte tag.
     */
    @Override
    public void encrypt(InputStream input, OutputStream output, String passphrase)
            throws Exception {
        validateKey(passphrase);

        // 1. Generate random IV
        byte[] iv = new byte[IV_SIZE_BYTES];
        new SecureRandom().nextBytes(iv);

        // 2. Write IV to output first
        output.write(iv);

        // 3. Derive key
        SecretKey key = deriveKey(passphrase);

        // 4. Initialise cipher in encrypt mode
        Cipher cipher = Cipher.getInstance(CIPHER_ALGO);
        GCMParameterSpec spec = new GCMParameterSpec(TAG_SIZE_BITS, iv);
        cipher.init(Cipher.ENCRYPT_MODE, key, spec);

        // 5. Stream plaintext → ciphertext (tag appended at end by JCA)
        streamThroughCipher(cipher, input, output);
    }

    /**
     * Decrypts using AES-256-GCM.
     *
     * Steps:
     *   1. Read the first 12 bytes as the IV.
     *   2. Derive the 256-bit key from the passphrase.
     *   3. Decrypt; JCA automatically verifies the tag and throws
     *      AEADBadTagException if it does not match.
     */
    @Override
    public void decrypt(InputStream input, OutputStream output, String passphrase)
            throws Exception {
        validateKey(passphrase);

        // 1. Read IV from stream header
        byte[] iv = new byte[IV_SIZE_BYTES];
        int bytesRead = input.read(iv);
        if (bytesRead != IV_SIZE_BYTES) {
            throw new IllegalArgumentException(
                "File too short – missing IV header. Not an encrypted file?");
        }

        // 2. Derive key
        SecretKey key = deriveKey(passphrase);

        // 3. Initialise cipher in decrypt mode
        Cipher cipher = Cipher.getInstance(CIPHER_ALGO);
        GCMParameterSpec spec = new GCMParameterSpec(TAG_SIZE_BITS, iv);
        cipher.init(Cipher.DECRYPT_MODE, key, spec);

        // 4. Stream ciphertext → plaintext (JCA verifies tag automatically)
        streamThroughCipher(cipher, input, output);
    }

    // ── Private helpers ──────────────────────────────────────────────

    /**
     * Derives a 256-bit AES key from a passphrase using SHA-256.
     * Matches the C implementation's aes_gcm_derive_key() exactly.
     */
    private SecretKey deriveKey(String passphrase) throws Exception {
        MessageDigest sha256 = MessageDigest.getInstance("SHA-256");
        byte[] keyBytes = sha256.digest(passphrase.getBytes("UTF-8"));
        return new SecretKeySpec(keyBytes, "AES");
    }

    /**
     * Reads from {@code input} in 4 KB chunks, pushes each chunk
     * through the cipher, and writes the output to {@code output}.
     * Calls {@link Cipher#doFinal()} to flush remaining bytes and
     * (for GCM decrypt) verify the authentication tag.
     */
    private void streamThroughCipher(Cipher cipher,
                                      InputStream input,
                                      OutputStream output) throws Exception {
        byte[] buffer    = new byte[4096];
        int    bytesRead;

        // Process data chunks
        while ((bytesRead = input.read(buffer)) != -1) {
            byte[] chunk = cipher.update(buffer, 0, bytesRead);
            if (chunk != null) {
                output.write(chunk);
            }
        }

        // Finalise: flushes remaining bytes; for decrypt, verifies GCM tag
        byte[] finalBlock = cipher.doFinal();
        if (finalBlock != null) {
            output.write(finalBlock);
        }
    }
}
