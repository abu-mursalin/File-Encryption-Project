/**
 * module-info.java
 * ----------------
 * Java Module System descriptor.
 * Updated for compatibility with Java 21+ / Java 24.
 */
module com.fileencryptor {
    // JavaFX modules needed at runtime
    requires javafx.controls;
    requires javafx.fxml;
    requires javafx.graphics;

    // Java standard modules
    requires java.base;
    requires java.logging;

    // Export all packages so JavaFX reflection works
    exports com.fileencryptor;
    exports com.fileencryptor.model;
    exports com.fileencryptor.cipher;
    exports com.fileencryptor.service;
    exports com.fileencryptor.controller;
    exports com.fileencryptor.view;
    exports com.fileencryptor.util;

    // Open to JavaFX for FXML and CSS reflection
    opens com.fileencryptor         to javafx.graphics, javafx.fxml;
    opens com.fileencryptor.view    to javafx.graphics, javafx.fxml;
    opens com.fileencryptor.model   to javafx.base;
}
