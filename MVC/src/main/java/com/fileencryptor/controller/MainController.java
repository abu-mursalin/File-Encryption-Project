package com.fileencryptor.controller;

import com.fileencryptor.model.Algorithm;
import com.fileencryptor.model.CryptoRequest;
import com.fileencryptor.model.CryptoResult;
import com.fileencryptor.service.CryptoService;
import com.fileencryptor.service.ILogger;
import com.fileencryptor.view.MainView;
import javafx.concurrent.Task;
import javafx.stage.FileChooser;

import java.io.File;

/**
 * MainController.java
 * -------------------
 * MVC Controller – mediates between {@link MainView} and {@link CryptoService}.
 *
 * Responsibilities:
 *   - Receives user-initiated events from the view.
 *   - Validates UI inputs and builds a {@link CryptoRequest}.
 *   - Runs the crypto operation on a background JavaFX Task so the UI
 *     remains responsive during large-file operations.
 *   - Interprets the {@link CryptoResult} and instructs the view to update.
 *   - Never contains JavaFX layout code (that belongs to the view).
 *   - Never contains cipher logic (that belongs to the service/strategy).
 *
 * SOLID – Single Responsibility:
 *   Pure coordination logic only.
 *
 * SOLID – Dependency Inversion:
 *   Depends on the CryptoService (which itself depends on interfaces),
 *   not on concrete cipher classes.
 */
public class MainController {

    // ── Injected dependencies ────────────────────────────────────────
    private final MainView      view;
    private final CryptoService cryptoService;
    private final ILogger       logger;

    // ── UI State ─────────────────────────────────────────────────────
    private File    selectedInputFile;
    private File    selectedOutputFile;

    /**
     * Constructs the controller and wires all view-event handlers.
     *
     * @param view          the JavaFX view (only interface methods are called)
     * @param cryptoService the service layer
     * @param logger        the logger
     */
    public MainController(MainView view,
                          CryptoService cryptoService,
                          ILogger logger) {
        this.view          = view;
        this.cryptoService = cryptoService;
        this.logger        = logger;

        // Register callbacks – the view calls these lambdas on user action
        view.setOnBrowseInput   (this::handleBrowseInput);
        view.setOnBrowseOutput  (this::handleBrowseOutput);
        view.setOnRun           (this::handleRun);
        view.setOnClear         (this::handleClear);
        view.setOnAlgorithmChange(this::handleAlgorithmChange);
    }

    // ── Event handlers (called by the view) ──────────────────────────

    /**
     * Opens a file-open dialog and stores the selected input file.
     */
    private void handleBrowseInput() {
        FileChooser chooser = new FileChooser();
        chooser.setTitle("Select Input File");
        File file = chooser.showOpenDialog(view.getWindow());
        if (file != null) {
            selectedInputFile = file;
            view.setInputFilePath(file.getAbsolutePath());
            logger.info("Input file selected: " + file.getAbsolutePath());
            checkReadiness();
        }
    }

    /**
     * Opens a file-save dialog and stores the selected output path.
     */
    private void handleBrowseOutput() {
        FileChooser chooser = new FileChooser();
        chooser.setTitle("Select Output File");
        File file = chooser.showSaveDialog(view.getWindow());
        if (file != null) {
            selectedOutputFile = file;
            view.setOutputFilePath(file.getAbsolutePath());
            logger.info("Output file selected: " + file.getAbsolutePath());
            checkReadiness();
        }
    }

    /**
     * Handles algorithm combo-box changes: updates the key-field placeholder.
     */
    private void handleAlgorithmChange(String selectedName) {
        Algorithm algo = Algorithm.fromDisplayName(selectedName);
        if (algo != null) {
            view.setKeyHint(algo.getKeyHint());
        }
    }

    /**
     * Validates all inputs, builds a {@link CryptoRequest}, and runs the
     * crypto operation on a background thread via a JavaFX {@link Task}.
     */
    private void handleRun() {

        // ── Collect and validate UI values ───────────────────────────
        String  passphrase = view.getPassphrase();
        boolean isEncrypt  = view.isEncryptSelected();
        String  algoName   = view.getSelectedAlgorithm();

        if (passphrase == null || passphrase.isBlank()) {
            view.showResult("Please enter a key or passphrase.");
            return;
        }
        if (!view.isActionSelected()) {
            view.showResult("Please select Encrypt or Decrypt.");
            return;
        }
        if (algoName == null || algoName.equals("Select an algorithm")) {
            view.showResult("Please select an algorithm.");
            return;
        }
        if (selectedInputFile == null) {
            view.showResult("Please choose an input file.");
            return;
        }
        if (selectedOutputFile == null) {
            view.showResult("Please choose an output file.");
            return;
        }

        Algorithm algorithm = Algorithm.fromDisplayName(algoName);
        if (algorithm == null) {
            view.showResult(CryptoResult.ERR_UNKNOWN_ALGO.getMessage());
            return;
        }

        // ── Build immutable request ──────────────────────────────────
        CryptoRequest request = new CryptoRequest(
                selectedInputFile,
                selectedOutputFile,
                algorithm,
                passphrase,
                isEncrypt
        );

        // ── Run on background thread so UI stays responsive ──────────
        view.setRunning(true);
        view.setStatus("Status: Running…");

        Task<CryptoResult> task = new Task<>() {
            @Override
            protected CryptoResult call() {
                return cryptoService.execute(request);
            }
        };

        task.setOnSucceeded(e -> {
            CryptoResult result = task.getValue();
            view.setRunning(false);
            view.showResult(result.getMessage());
            if (result == CryptoResult.OK) {
                view.setStatus("Status: Done ✓");
                logger.info("Operation succeeded.");
            } else {
                view.setStatus("Status: Failed ✗");
                logger.error("Operation failed: " + result.getMessage());
            }
        });

        task.setOnFailed(e -> {
            view.setRunning(false);
            view.setStatus("Status: Error");
            view.showResult("Unexpected error: " + task.getException().getMessage());
            logger.error("Task failed: " + task.getException().getMessage());
        });

        Thread thread = new Thread(task);
        thread.setDaemon(true);  // allow JVM to exit even if task is running
        thread.start();
    }

    /**
     * Resets all state and clears the view back to its initial state.
     */
    private void handleClear() {
        selectedInputFile  = null;
        selectedOutputFile = null;
        view.reset();
        logger.info("UI cleared by user.");
    }

    // ── Private helpers ──────────────────────────────────────────────

    /**
     * Enables the Run button once both files are chosen and an action
     * and algorithm have been selected.
     */
    private void checkReadiness() {
        boolean ready = selectedInputFile != null &&
                        selectedOutputFile != null &&
                        view.isActionSelected() &&
                        view.getSelectedAlgorithm() != null;
        if (ready) {
            view.setStatus("Status: Ready");
        }
    }
}
