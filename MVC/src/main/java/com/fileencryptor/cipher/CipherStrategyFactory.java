package com.fileencryptor.cipher;

import com.fileencryptor.model.Algorithm;

/**
 * CipherStrategyFactory.java
 * --------------------------
 * Factory that maps an {@link Algorithm} enum value to its
 * concrete {@link ICipherStrategy} implementation.
 *
 * SOLID – Open/Closed:
 *   To add a new algorithm, add an enum constant in Algorithm and a
 *   new case here.  No other existing class needs modification.
 *
 * SOLID – Dependency Inversion:
 *   Callers ask for an ICipherStrategy and never import concrete
 *   cipher classes directly.
 *
 * SOLID – Single Responsibility:
 *   Pure factory logic only – no validation, no I/O.
 */
public final class CipherStrategyFactory {

    // Non-instantiable utility class
    private CipherStrategyFactory() {}

    /**
     * Returns the {@link ICipherStrategy} that corresponds to {@code algorithm}.
     *
     * @param algorithm the chosen algorithm enum value
     * @return          a fresh strategy instance ready for use
     * @throws IllegalArgumentException if no strategy is registered for the given algorithm
     */
    public static ICipherStrategy create(Algorithm algorithm) {
        switch (algorithm) {
            case CAESAR_CIPHER: return new CaesarCipherStrategy();
            case XOR_CIPHER:    return new XorCipherStrategy();
            case AES_256_GCM:   return new AesGcmCipherStrategy();
            default:
                throw new IllegalArgumentException(
                    "No strategy registered for algorithm: " + algorithm);
        }
    }
}
