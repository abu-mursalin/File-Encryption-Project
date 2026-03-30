package com.fileencryptor;

import com.fileencryptor.controller.MainController;
import com.fileencryptor.service.CryptoService;
import com.fileencryptor.service.FileHandlerImpl;
import com.fileencryptor.util.FileLogger;
import com.fileencryptor.view.MainView;
import javafx.application.Application;
import javafx.stage.Stage;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * App.java
 * --------
 * Entry point. Resolves log file path based on environment:
 *
 *   mvn javafx:run   → project root\file_encryption.log
 *   installed .exe   → C:\Users\<username>\FileEncryptor\file_encryption.log
 */
public class App extends Application {

    private FileLogger logger;

    @Override
    public void start(Stage primaryStage) {

        // ── 1. Resolve log path ──────────────────────────────────────
        String logPath = resolveLogPath();

        // ── 2. Start logger ──────────────────────────────────────────
        logger = new FileLogger(logPath);
        logger.info("=== File Encryption Application starting (JavaFX) ===");
        logger.info("Algorithms available: Caesar Cipher, XOR Cipher, AES-256-GCM");
        logger.info("Log file: " + logPath);

        // ── 3. Construct dependency graph ────────────────────────────
        FileHandlerImpl fileHandler   = new FileHandlerImpl();
        CryptoService   cryptoService = new CryptoService(fileHandler, logger);

        // ── 4. Build View and Controller ─────────────────────────────
        MainView       view       = new MainView(primaryStage);
        MainController controller = new MainController(view, cryptoService, logger);

        // ── 5. Show window ───────────────────────────────────────────
        primaryStage.show();
        logger.info("Application window displayed.");
    }

    @Override
    public void stop() {
        if (logger != null) {
            logger.info("Application shutting down.");
            logger.close();
        }
    }

    public static void main(String[] args) {
        launch(args);
    }

    // ── Private helpers ──────────────────────────────────────────────

    /**
     * Resolves where to save the log file.
     *
     * Priority order:
     *   1. Current working directory  — works for mvn javafx:run
     *   2. C:\Users\<user>\FileEncryptor\  — works for installed .exe
     *   3. Just the filename as fallback
     *
     * The installed .exe always lands at:
     *   C:\Users\<username>\FileEncryptor\
     * which is fully writable — so priority 2 always works after install.
     */
    private String resolveLogPath() {
        String logFileName = "file_encryption.log";

        // ── Priority 1: current working directory ────────────────────
        // Works when running via mvn javafx:run from project folder
        File currentDir    = new File(System.getProperty("user.dir"));
        File logInCurrent  = new File(currentDir, logFileName);
        try {
            if (logInCurrent.exists() && logInCurrent.canWrite()) {
                return logInCurrent.getAbsolutePath();
            }
            if (!logInCurrent.exists() && logInCurrent.createNewFile()) {
                return logInCurrent.getAbsolutePath();
            }
        } catch (Exception ignored) { }

        // ── Priority 2: C:\Users\<username>\FileEncryptor\ ───────────
        // This is exactly where the installer puts the app, so this
        // folder already exists and is always writable by the user.
        try {
            Path userAppDir = Paths.get(
                System.getProperty("user.home"), "FileEncryptor");
            Files.createDirectories(userAppDir);
            File logFile = userAppDir.resolve(logFileName).toFile();
            return logFile.getAbsolutePath();
        } catch (Exception ignored) { }

        // ── Priority 3: fallback ─────────────────────────────────────
        return logFileName;
    }
}
