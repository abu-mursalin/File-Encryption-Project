package com.fileencryptor.model;

/**
 * CryptoResult.java
 * -----------------
 * Represents the outcome of an encrypt/decrypt operation.
 *
 * SOLID – Single Responsibility:
 *   This enum only models result states and their human-readable messages.
 *   No logic, no I/O, no UI dependencies.
 */
public enum CryptoResult {

    OK                ("Operation completed successfully."),
    ERR_INVALID_KEY   ("Invalid key – please check the key constraints."),
    ERR_UNKNOWN_ALGO  ("Unknown algorithm selected."),
    ERR_IO            ("I/O error during the operation."),
    ERR_AUTH_FAILED   ("Authentication failed – wrong key or tampered file."),
    ERR_NULL_PARAM    ("Internal error: a required parameter was null.");

    private final String message;

    CryptoResult(String message) {
        this.message = message;
    }

    /** Returns the human-readable description of this result code. */
    public String getMessage() {
        return message;
    }
}
