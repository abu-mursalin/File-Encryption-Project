package com.fileencryptor.view;

import com.fileencryptor.model.Algorithm;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.stage.Stage;
import javafx.stage.Window;

import java.util.function.Consumer;

/**
 * MainView.java
 * -------------
 * MVC View – builds and owns every JavaFX widget.
 *
 * Rules:
 *   - No business logic here; the controller drives all behaviour.
 *   - Communicates with the controller via plain Java callbacks
 *     (Runnable / Consumer), so no JavaFX types leak into the controller.
 *
 * SOLID – Single Responsibility:
 *   Solely responsible for layout, widget creation, and exposing
 *   a narrow read/write API to the controller.
 */
public class MainView {

    // ── Widgets exposed to the controller via the API ─────────────────
    private final Stage        stage;
    private final TextField    inputPathField;
    private final TextField    outputPathField;
    private final PasswordField passphraseField;
    private final CheckBox      showPassCheckBox;
    private final TextField     passphraseVisible;   // swapped in when "show" ticked
    private final ComboBox<String> algoCombo;
    private final RadioButton   encryptRadio;
    private final RadioButton   decryptRadio;
    private final Button        runButton;
    private final Button        clearButton;
    private final Button        browseInputBtn;
    private final Button        browseOutputBtn;
    private final Label         statusLabel;
    private final Label         resultLabel;

    // ── Controller callbacks ──────────────────────────────────────────
    private Runnable         onBrowseInput;
    private Runnable         onBrowseOutput;
    private Runnable         onRun;
    private Runnable         onClear;
    private Consumer<String> onAlgorithmChange;

    // ── Constructor ──────────────────────────────────────────────────

    public MainView(Stage stage) {
        this.stage = stage;

        // ── Instantiate all controls ─────────────────────────────────
        inputPathField    = buildPathField("Input file path will appear here…");
        outputPathField   = buildPathField("Output file path will appear here…");
        passphraseField   = new PasswordField();
        passphraseVisible = new TextField();
        showPassCheckBox  = new CheckBox("Show");
        algoCombo         = new ComboBox<>();
        encryptRadio      = new RadioButton("Encrypt");
        decryptRadio      = new RadioButton("Decrypt");
        runButton         = new Button("Run");
        clearButton       = new Button("Clear");
        browseInputBtn    = new Button("Browse…");
        browseOutputBtn   = new Button("Browse…");
        statusLabel       = new Label("Status: Waiting for input…");
        resultLabel       = new Label("");

        // ── Wire internal widget events ──────────────────────────────
        wireInternalEvents();

        // ── Build and show scene ─────────────────────────────────────
        stage.setScene(buildScene());
        stage.setTitle("File Encryptor — JavaFX");
        stage.setMinWidth(680);
        stage.setMinHeight(420);
    }

    // ── Scene builder ─────────────────────────────────────────────────

    private Scene buildScene() {
        VBox root = new VBox(14);
        root.setPadding(new Insets(30));
        root.getStyleClass().add("root-pane");

        root.getChildren().addAll(
            buildTitle(),
            buildSeparator(),
            buildInputRow(),
            buildOutputRow(),
            buildAlgorithmRow(),
            buildActionRow(),
            buildPassphraseRow(),
            buildSeparator(),
            buildControlRow(),
            resultLabel
        );

        resultLabel.getStyleClass().add("result-label");
        resultLabel.setWrapText(true);

        Scene scene = new Scene(root, 720, 460);
        scene.getStylesheets().add(
            getClass().getResource("/com/fileencryptor/css/style.css").toExternalForm());
        return scene;
    }

    private Label buildTitle() {
        Label title = new Label("🔐  File Encryption Tool");
        title.getStyleClass().add("app-title");
        return title;
    }

    private Separator buildSeparator() {
        return new Separator();
    }

    // ── File row (label + path display + browse button) ──────────────

    private HBox buildInputRow() {
        Label lbl = new Label("Input File:");
        lbl.getStyleClass().add("field-label");
        lbl.setMinWidth(110);
        HBox.setHgrow(inputPathField, Priority.ALWAYS);
        HBox row = new HBox(10, lbl, inputPathField, browseInputBtn);
        row.setAlignment(Pos.CENTER_LEFT);
        return row;
    }

    private HBox buildOutputRow() {
        Label lbl = new Label("Output File:");
        lbl.getStyleClass().add("field-label");
        lbl.setMinWidth(110);
        HBox.setHgrow(outputPathField, Priority.ALWAYS);
        HBox row = new HBox(10, lbl, outputPathField, browseOutputBtn);
        row.setAlignment(Pos.CENTER_LEFT);
        return row;
    }

    // ── Algorithm combo ───────────────────────────────────────────────

    private HBox buildAlgorithmRow() {
        Label lbl = new Label("Algorithm:");
        lbl.getStyleClass().add("field-label");
        lbl.setMinWidth(110);

        algoCombo.getItems().add("Select an algorithm");
        for (Algorithm a : Algorithm.values()) {
            algoCombo.getItems().add(a.getDisplayName());
        }
        algoCombo.getSelectionModel().selectFirst();
        algoCombo.setMaxWidth(Double.MAX_VALUE);
        HBox.setHgrow(algoCombo, Priority.ALWAYS);

        HBox row = new HBox(10, lbl, algoCombo);
        row.setAlignment(Pos.CENTER_LEFT);
        return row;
    }

    // ── Encrypt / Decrypt radio buttons ──────────────────────────────

    private HBox buildActionRow() {
        Label lbl = new Label("Action:");
        lbl.getStyleClass().add("field-label");
        lbl.setMinWidth(110);

        ToggleGroup group = new ToggleGroup();
        encryptRadio.setToggleGroup(group);
        decryptRadio.setToggleGroup(group);
        encryptRadio.getStyleClass().add("action-radio");
        decryptRadio.getStyleClass().add("action-radio");

        HBox row = new HBox(20, lbl, encryptRadio, decryptRadio);
        row.setAlignment(Pos.CENTER_LEFT);
        return row;
    }

    // ── Passphrase row with show/hide toggle ─────────────────────────

    private HBox buildPassphraseRow() {
        Label lbl = new Label("Passphrase:");
        lbl.getStyleClass().add("field-label");
        lbl.setMinWidth(110);

        passphraseField.setPromptText("Enter key / passphrase…");
        passphraseVisible.setPromptText("Enter key / passphrase…");
        passphraseVisible.setVisible(false);
        passphraseVisible.setManaged(false);

        HBox.setHgrow(passphraseField,   Priority.ALWAYS);
        HBox.setHgrow(passphraseVisible, Priority.ALWAYS);

        StackPane passStack = new StackPane(passphraseField, passphraseVisible);
        HBox.setHgrow(passStack, Priority.ALWAYS);

        HBox row = new HBox(10, lbl, passStack, showPassCheckBox);
        row.setAlignment(Pos.CENTER_LEFT);
        return row;
    }

    // ── Control row (status + buttons) ───────────────────────────────

    private HBox buildControlRow() {
        statusLabel.getStyleClass().add("status-label");
        statusLabel.setMaxWidth(Double.MAX_VALUE);
        HBox.setHgrow(statusLabel, Priority.ALWAYS);

        runButton.getStyleClass().add("run-button");
        clearButton.getStyleClass().add("clear-button");

        HBox row = new HBox(10, statusLabel, runButton, clearButton);
        row.setAlignment(Pos.CENTER_LEFT);
        return row;
    }

    // ── Internal widget wiring (no controller callbacks yet) ──────────

    private void wireInternalEvents() {
        // Show/hide passphrase toggle
        showPassCheckBox.setOnAction(e -> {
            boolean show = showPassCheckBox.isSelected();
            if (show) {
                passphraseVisible.setText(passphraseField.getText());
                passphraseField.setVisible(false);
                passphraseField.setManaged(false);
                passphraseVisible.setVisible(true);
                passphraseVisible.setManaged(true);
            } else {
                passphraseField.setText(passphraseVisible.getText());
                passphraseVisible.setVisible(false);
                passphraseVisible.setManaged(false);
                passphraseField.setVisible(true);
                passphraseField.setManaged(true);
            }
        });

        // Delegate all button/combo events to controller callbacks
        browseInputBtn.setOnAction (e -> { if (onBrowseInput    != null) onBrowseInput.run(); });
        browseOutputBtn.setOnAction(e -> { if (onBrowseOutput   != null) onBrowseOutput.run(); });
        runButton.setOnAction      (e -> { if (onRun            != null) onRun.run(); });
        clearButton.setOnAction    (e -> { if (onClear          != null) onClear.run(); });

        algoCombo.setOnAction(e -> {
            if (onAlgorithmChange != null) {
                onAlgorithmChange.accept(algoCombo.getValue());
            }
        });
    }

    // ── Controller callback setters ───────────────────────────────────

    public void setOnBrowseInput   (Runnable         cb) { this.onBrowseInput    = cb; }
    public void setOnBrowseOutput  (Runnable         cb) { this.onBrowseOutput   = cb; }
    public void setOnRun           (Runnable         cb) { this.onRun            = cb; }
    public void setOnClear         (Runnable         cb) { this.onClear          = cb; }
    public void setOnAlgorithmChange(Consumer<String> cb) { this.onAlgorithmChange = cb; }

    // ── Read API (used by controller to gather inputs) ────────────────

    public String  getPassphrase()       {
        return showPassCheckBox.isSelected()
            ? passphraseVisible.getText()
            : passphraseField.getText();
    }
    public boolean isEncryptSelected()   { return encryptRadio.isSelected(); }
    public boolean isActionSelected()    { return encryptRadio.isSelected() || decryptRadio.isSelected(); }
    public String  getSelectedAlgorithm(){ return algoCombo.getValue(); }
    public Window  getWindow()           { return stage; }

    // ── Write API (used by controller to update UI) ───────────────────

    public void setInputFilePath (String path) { inputPathField.setText(path);  }
    public void setOutputFilePath(String path) { outputPathField.setText(path); }
    public void setStatus        (String text) { statusLabel.setText(text);     }
    public void showResult       (String text) { resultLabel.setText(text);     }
    public void setKeyHint       (String hint) {
        passphraseField.setPromptText(hint);
        passphraseVisible.setPromptText(hint);
    }

    /**
     * Disables/enables interactive controls during a long-running operation.
     * Prevents the user from starting a second operation concurrently.
     */
    public void setRunning(boolean running) {
        runButton.setDisable(running);
        clearButton.setDisable(running);
        browseInputBtn.setDisable(running);
        browseOutputBtn.setDisable(running);
        algoCombo.setDisable(running);
        encryptRadio.setDisable(running);
        decryptRadio.setDisable(running);
    }

    /**
     * Resets all widgets to their initial state (called by Clear button).
     */
    public void reset() {
        inputPathField.clear();
        outputPathField.clear();
        passphraseField.clear();
        passphraseVisible.clear();
        algoCombo.getSelectionModel().selectFirst();
        encryptRadio.setSelected(false);
        decryptRadio.setSelected(false);
        showPassCheckBox.setSelected(false);
        passphraseField.setVisible(true);
        passphraseField.setManaged(true);
        passphraseVisible.setVisible(false);
        passphraseVisible.setManaged(false);
        passphraseField.setPromptText("Enter key / passphrase…");
        passphraseVisible.setPromptText("Enter key / passphrase…");
        statusLabel.setText("Status: Waiting for input…");
        resultLabel.setText("");
        setRunning(false);
    }

    // ── Private helper ────────────────────────────────────────────────

    private TextField buildPathField(String prompt) {
        TextField tf = new TextField();
        tf.setPromptText(prompt);
        tf.setEditable(false);
        tf.getStyleClass().add("path-field");
        return tf;
    }
}
