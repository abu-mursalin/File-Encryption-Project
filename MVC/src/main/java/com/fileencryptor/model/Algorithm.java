package com.fileencryptor.model;

/**
 * Algorithm.java
 * --------------
 * Enumerates the supported encryption algorithms.
 * Each value carries the display name shown in the combo-box
 * and the constraint hint shown as a key-field placeholder.
 *
 * SOLID – Single Responsibility:
 *   Pure data enumeration – no logic, no file access.
 *
 * SOLID – Open/Closed:
 *   Adding a new algorithm requires only a new enum constant
 *   plus one new ICipherStrategy implementation; no existing
 *   code needs to be modified.
 */
public enum Algorithm {

    CAESAR_CIPHER ("Caesar Cipher", "Enter shift key (1 – 25)"),
    XOR_CIPHER    ("XOR Cipher",    "Enter XOR key (1 – 255)"),
    AES_256_GCM   ("AES-256-GCM",  "Enter passphrase (AES-256-GCM)");

    // ── Display / UI strings ─────────────────────────────────────────
    private final String displayName;
    private final String keyHint;

    Algorithm(String displayName, String keyHint) {
        this.displayName = displayName;
        this.keyHint     = keyHint;
    }

    /** Label shown in the algorithm combo-box. */
    public String getDisplayName() { return displayName; }

    /** Placeholder shown inside the key/passphrase text field. */
    public String getKeyHint()     { return keyHint;     }

    /**
     * Looks up an Algorithm by its display name.
     * Returns null if not found (caller handles the unknown-algorithm case).
     */
    public static Algorithm fromDisplayName(String name) {
        for (Algorithm a : values()) {
            if (a.displayName.equals(name)) return a;
        }
        return null;
    }
}
