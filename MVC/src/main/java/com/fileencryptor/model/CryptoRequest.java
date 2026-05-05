package com.fileencryptor.model;

import java.io.File;

/**
 * CryptoRequest.java
 * ------------------
 * Immutable value object that bundles all inputs needed for one
 * encrypt/decrypt operation.  Passed from the Controller to the
 * CryptoService so that neither depends on JavaFX UI types.
 *
 * SOLID – Single Responsibility:
 *   Only carries data – no behaviour, no validation logic.
 *
 * SOLID – Dependency Inversion:
 *   The service layer depends on this plain model object rather
 *   than on concrete JavaFX widgets or GTK widgets.
 */
public final class CryptoRequest {

    private final File      inputFile;
    private final File      outputFile;
    private final Algorithm algorithm;
    private final String    passphrase;
    private final boolean   encrypt;   // true = encrypt, false = decrypt

    public CryptoRequest(File inputFile,
                         File outputFile,
                         Algorithm algorithm,
                         String passphrase,
                         boolean encrypt) {
        this.inputFile  = inputFile;
        this.outputFile = outputFile;
        this.algorithm  = algorithm;
        this.passphrase = passphrase;
        this.encrypt    = encrypt;
    }

    public File      getInputFile()  { return inputFile;  }
    public File      getOutputFile() { return outputFile; }
    public Algorithm getAlgorithm()  { return algorithm;  }
    public String    getPassphrase() { return passphrase; }
    public boolean   isEncrypt()     { return encrypt;    }
}
