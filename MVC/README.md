# 🔐 File Encryptor — Java/JavaFX Edition

![Java](https://img.shields.io/badge/Language-Java%2021-orange?style=flat-square&logo=openjdk)
![JavaFX](https://img.shields.io/badge/GUI-JavaFX%2021-blue?style=flat-square)
![Maven](https://img.shields.io/badge/Build-Maven%203.8-red?style=flat-square&logo=apachemaven)
![Architecture](https://img.shields.io/badge/Architecture-MVC-green?style=flat-square)
![SOLID](https://img.shields.io/badge/Principles-SOLID-purple?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-yellow?style=flat-square)

A GUI-based file encryption and decryption desktop application built in **Java 21** using **JavaFX 21** for the graphical interface and the **Java Cryptography Architecture (JCA)** for cryptographic operations. Supports **Caesar Cipher**, **XOR Cipher**, and **AES-256-GCM** encryption algorithms.

This project is a **complete conversion** of the original **C/GTK4** implementation into a modern Java application following **MVC architecture**, all five **SOLID principles**, full **OOP**, and clean modular design with interfaces at every boundary.

> **Course Code:** 0714 02 CSE 2100
> **Course Title:** Advanced Programming Laboratory
> **Contributors:** MD. Abu Mursalin (ID: 240212) · Udoy Munna (ID: 240229)

---

## 📋 Table of Contents

- [Conversion Brief](#conversion-brief)
- [Overview](#overview)
- [Features](#features)
- [System Architecture](#system-architecture)
- [Project Structure](#project-structure)
- [Algorithms](#algorithms)
- [SOLID Principles Applied](#solid-principles-applied)
- [Design Patterns](#design-patterns)
- [UML Class Diagram](#uml-class-diagram)
- [C vs Java — Full Comparison](#c-vs-java--full-comparison)
- [Dependencies & Requirements](#dependencies--requirements)
- [Building & Running](#building--running)
- [Creating a Windows Installer (.exe)](#creating-a-windows-installer-exe)
- [Usage Guide](#usage-guide)
- [Log File Location](#log-file-location)
- [Security Design](#security-design)
- [Known Limitations](#known-limitations)
- [Future Improvements](#future-improvements)
- [Resource Links](#resource-links)

---

## Conversion Brief

This project was created in response to the following conversion requirement:

> *"I have a file encryption project written in C that includes a graphical user interface (GUI). I want to convert it into a Java application using JavaFX."*

**Prompts that were used:**

| # | Prompts | How Fulfilled |
|---|---|---|
| 1 | Convert full project (logic + GUI) into Java | All 8 C modules converted to 20 Java classes |
| 2 | Use JavaFX for GUI (buttons, file chooser, text fields) | `MainView.java` — full JavaFX UI with FileChooser, PasswordField, ComboBox, RadioButton |
| 3 | Follow Object-Oriented Programming (OOP) | Encapsulation, inheritance, polymorphism used throughout |
| 4 | Follow SOLID principles | All 5 principles applied — see [SOLID Principles Applied](#solid-principles-applied) |
| 5 | Use MVC or layered architecture | Full MVC — `MainView` / `MainController` / `CryptoService` + model layer |
| 6 | Separate GUI, business logic, and file handling | View / Controller / Service / Cipher — 4 completely separate layers |
| 7 | Make the code clean, modular, and scalable | 7 packages, 20 classes, each with a single responsibility |
| 8 | Use interfaces to reduce coupling | `ICipherStrategy`, `IFileHandler`, `ILogger` — all boundaries are interfaces |
| 9 | Add comments for understanding | Every class, method, and complex block has Javadoc + inline comments |
| 10 | Add basic styling (CSS) for GUI | `style.css` — full dark Catppuccin theme |

---

## Overview

The original C project used **GTK4** for the GUI and **OpenSSL** for cryptography, compiled with **GCC** and built using **Make**. This Java version replaces every component with its modern Java equivalent:

| C Component | Java Equivalent |
|---|---|
| GTK4 widgets | JavaFX controls |
| OpenSSL EVP API | Java JCA (`javax.crypto`) |
| `struct AppState` | `CryptoRequest` value object |
| `crypto_dispatcher.c` | `CryptoService` + `CipherStrategyFactory` |
| `ui.c` signal callbacks | Controller callbacks (lambdas) |
| `Makefile` | `pom.xml` (Maven) |
| `make` / `make clean` | `mvn javafx:run` / `mvn clean` |

The Java version adds features the C version did not have: background thread execution (UI never freezes), automatic in-place file handling, smart log path resolution, a proper Windows installer, and a fully testable architecture via dependency injection.

---

## Features

- 🗂️ Browse and select any file for encryption or decryption
- 🔒 Three encryption algorithms: Caesar Cipher, XOR Cipher, AES-256-GCM
- 🛡️ AES-256-GCM with authenticated encryption (tamper detection)
- 🎲 Random 12-byte IV generated per encryption via `SecureRandom`
- 🔑 SHA-256 key derivation from passphrase to 32-byte AES key
- ✅ Same-file in-place encryption (read-all → cipher in memory → write back)
- 🔁 Background thread execution — UI stays responsive during large file operations
- 👁️ Show / hide passphrase toggle
- 🧹 One-click Clear to reset all fields
- 💬 Descriptive status and result messages for every operation
- 📝 Timestamped logging to `stderr` and `file_encryption.log`
- 🎨 Dark theme CSS stylesheet (Catppuccin-inspired)
- 📦 jpackage support — builds a real Windows `.exe` installer
- 🖥️ Runs on Windows, Linux, and macOS

---

## System Architecture

The application follows a strict **MVC + Layered** architecture:

```
┌─────────────────────────────────────────────────────────────┐
│           View Layer   (com.fileencryptor.view)              │
│   MainView — owns all JavaFX widgets; exposes a clean        │
│   read/write API to the controller; fires callbacks;         │
│   contains ZERO business logic                               │
└────────────────────────┬────────────────────────────────────┘
                         │  Runnable / Consumer<String> callbacks
┌────────────────────────▼────────────────────────────────────┐
│        Controller Layer  (com.fileencryptor.controller)      │
│   MainController — collects UI values; validates inputs;     │
│   builds CryptoRequest; runs Task on background thread;      │
│   updates view with CryptoResult                             │
└────────────────────────┬────────────────────────────────────┘
                         │  CryptoRequest in / CryptoResult out
┌────────────────────────▼────────────────────────────────────┐
│          Service Layer  (com.fileencryptor.service)          │
│   CryptoService — detects in-place vs two-file; opens        │
│   streams; calls cipher strategy; maps exceptions to         │
│   CryptoResult; always closes streams in finally block       │
└──────────┬──────────────────────────┬───────────────────────┘
           │                          │
┌──────────▼──────────┐   ┌──────────▼──────────────────────┐
│   Cipher Layer       │   │   Util / Infra Layer            │
│   ICipherStrategy    │   │   IFileHandler → FileHandlerImpl │
│   Caesar / XOR / AES │   │   ILogger     → FileLogger      │
└─────────────────────┘   └─────────────────────────────────┘
```

**Data flow for one encrypt operation:**

```
User clicks Run
  → MainController collects: file paths, algorithm, passphrase, action
  → Builds CryptoRequest (immutable value object)
  → Spawns JavaFX Task on background thread
      → CryptoService.execute(request)
          → CipherStrategyFactory.create(algorithm) → ICipherStrategy
          → strategy.validateKey(passphrase)
          → fileHandler.openRead(input) / fileHandler.openWrite(output)
          → strategy.encrypt(in, out, passphrase)
          → returns CryptoResult.OK
  → Task.setOnSucceeded → MainController updates MainView
  → MainView.showResult("Operation completed successfully.")
```

---

## MVC Architecture — In Depth

### What is MVC?

**Model–View–Controller (MVC)** is an architectural pattern that divides an application into three interconnected components. The core idea is **separation of concerns**: each component has a clearly defined role and knows as little as possible about the others.

```
┌─────────┐   user events    ┌────────────┐   business calls   ┌─────────┐
│         │ ───────────────► │            │ ──────────────────► │         │
│  View   │                  │ Controller │                     │  Model  │
│         │ ◄─────────────── │            │ ◄────────────────── │         │
└─────────┘   UI updates     └────────────┘   data / results   └─────────┘
```

| Component | Core Question | Rule |
|---|---|---|
| **Model** | *What is the data and logic?* | Never imports View or Controller classes |
| **View** | *What does the user see?* | Never contains business rules or I/O |
| **Controller** | *What happens when the user acts?* | Never constructs UI widgets directly |

---

### The Model Layer — `com.fileencryptor.model`

The Model represents the **state and domain knowledge** of the application. It is completely independent of how that data is displayed or how it is requested.

#### `Algorithm.java` — The algorithm enumeration

```java
public enum Algorithm {
    CAESAR_CIPHER("Caesar Cipher", "Key: integer 1–25"),
    XOR_CIPHER    ("XOR Cipher",   "Key: integer 1–255"),
    AES_256_GCM   ("AES-256-GCM",  "Passphrase: any string");

    private final String displayName;
    private final String keyHint;
    // getters, fromDisplayName() factory...
}
```

`Algorithm` encodes domain knowledge — what algorithms exist, what their human-readable names are, and what hint the UI should show for each one. It knows nothing about JavaFX, file streams, or cryptographic implementation details.

#### `CryptoRequest.java` — Immutable input bundle

```java
public final class CryptoRequest {
    private final File      inputFile;
    private final File      outputFile;
    private final Algorithm algorithm;
    private final String    passphrase;
    private final boolean   encrypt;
    // constructor + getters only — no setters
}
```

`CryptoRequest` is a **value object**: a bundle of all data needed to perform one crypto operation. It is immutable (all fields are `final`, no setters) which makes it **thread-safe by design** — the Controller can hand it off to a background thread with zero risk of concurrent modification.

This replaces the mutable `struct AppState` in the C version, which had to be carefully managed across GTK callbacks.

#### `CryptoResult.java` — Typed outcome model

```java
public enum CryptoResult {
    OK                ("Operation completed successfully."),
    ERR_INVALID_KEY   ("Invalid key or passphrase for the selected algorithm."),
    ERR_UNKNOWN_ALGO  ("Unknown or unsupported algorithm selected."),
    ERR_IO            ("File I/O error — check paths and permissions."),
    ERR_AUTH_FAILED   ("AES authentication failed — wrong passphrase or tampered file."),
    ERR_NULL_PARAM    ("A required input was null or empty.");

    private final String message;
    // getMessage()
}
```

Every possible outcome of a crypto operation is a named, typed constant with a human-readable message. The Controller never builds error strings itself — it simply reads `result.getMessage()` and forwards it to the View. This means all outcome messages live in one place, and changing a message never touches Controller or View code.

---

### The View Layer — `com.fileencryptor.view`

The View is **everything the user sees and touches**. It owns every JavaFX widget. Its only jobs are:

1. Build and lay out the UI
2. Expose a clean **read API** so the Controller can pull values out
3. Expose a clean **write API** so the Controller can push results in
4. Fire **callbacks** when the user acts — without knowing what those callbacks do

#### `MainView.java` — Structure

```
MainView
│
├── Widget ownership
│   ├── ComboBox<String>    algoCombo          ← algorithm selector
│   ├── RadioButton         encryptRadio       ← encrypt / decrypt toggle
│   ├── RadioButton         decryptRadio
│   ├── TextField           inputFileField     ← shows selected input path
│   ├── TextField           outputFileField    ← shows selected output path
│   ├── PasswordField       passphraseField    ← hidden passphrase input
│   ├── TextField           passphraseVisible  ← shown when eye-icon toggled
│   ├── Label               keyHintLabel       ← changes per algorithm
│   ├── Label               statusLabel        ← "Running…" / idle
│   ├── Label               resultLabel        ← success / error message
│   ├── Button              runButton
│   └── Button              clearButton
│
├── Read API (Controller pulls these)
│   ├── getInputFilePath()   : String
│   ├── getOutputFilePath()  : String
│   ├── getAlgorithm()       : String
│   ├── getPassphrase()      : String
│   └── isEncryptSelected()  : boolean
│
├── Write API (Controller pushes these)
│   ├── setInputFilePath(String)
│   ├── setOutputFilePath(String)
│   ├── showStatus(String)
│   ├── showResult(String)
│   ├── setRunning(boolean)  ← disables Run, shows progress
│   ├── setKeyHint(String)
│   └── reset()              ← clears all fields to defaults
│
└── Callback registration (Controller registers handlers)
    ├── setOnBrowseInput(Runnable)
    ├── setOnBrowseOutput(Runnable)
    ├── setOnRun(Runnable)
    ├── setOnClear(Runnable)
    └── setOnAlgorithmChange(Consumer<String>)
```

#### The Zero-Business-Logic Rule

`MainView` contains **no `if` statements that concern encryption, no file reading, no key validation**. When the user clicks *Run*, `MainView` does not decide whether the inputs are valid — it simply fires the registered `onRun` callback. The Controller decides what to do.

This means the View can be completely replaced (e.g. swapped for an FXML-based layout or a web front end) without touching a single line of Controller or Model code.

#### Callback-based decoupling

Instead of `MainView` directly referencing `MainController`, the View exposes callback registration slots:

```java
// MainView — exposes a slot, knows nothing about who fills it
public void setOnRun(Runnable handler) {
    runButton.setOnAction(e -> handler.run());
}

// MainController — fills the slot at wiring time
view.setOnRun(this::handleRun);
```

This is the **Observer pattern** applied at the View/Controller boundary. The View is observable; the Controller is the observer. Neither holds a direct reference to the other's implementation type.

#### Thread safety in the View

JavaFX requires all UI updates to happen on the **JavaFX Application Thread**. `MainView`'s write API methods (`showResult`, `showStatus`, `setRunning`) are designed to be called safely from `Task.setOnSucceeded` / `setOnFailed` callbacks, which JavaFX already marshals back to the Application Thread. This means the View never needs to call `Platform.runLater()` internally.

---

### The Controller Layer — `com.fileencryptor.controller`

The Controller is the **coordinator**. It sits between the View (what the user sees) and the Model/Service (what the application does). It contains no business logic itself — it collects, validates, assembles, dispatches, and updates.

#### `MainController.java` — Responsibilities

```
MainController
│
├── Wiring (done once in App.java)
│   └── Registers all callbacks on MainView
│
├── handleBrowseInput()
│   └── Opens JavaFX FileChooser
│   └── Calls view.setInputFilePath(chosen path)
│
├── handleBrowseOutput()
│   └── Opens JavaFX FileChooser (save mode)
│   └── Calls view.setOutputFilePath(chosen path)
│
├── handleAlgorithmChange(String algo)
│   └── Reads Algorithm from display name
│   └── Calls view.setKeyHint(algorithm.getKeyHint())
│
├── handleRun()   ← the core coordination method
│   ├── Pull values from view (getPassphrase, getAlgorithm, etc.)
│   ├── Validate inputs (null checks, empty checks)
│   │   └── On failure: view.showResult(error message) and return
│   ├── Build CryptoRequest (immutable)
│   ├── view.setRunning(true)   ← lock UI
│   ├── Create JavaFX Task<CryptoResult>
│   │   └── call() { return cryptoService.execute(request); }
│   ├── task.setOnSucceeded { view.showResult(...); view.setRunning(false); }
│   ├── task.setOnFailed    { view.showResult("Unexpected error."); ... }
│   └── new Thread(task).start()
│
└── handleClear()
    └── view.reset()
```

#### Input validation — Controller's only logic

The one piece of logic that belongs exclusively to the Controller is **input validation before dispatching**:

```java
private void handleRun() {
    String inputPath  = view.getInputFilePath();
    String passphrase = view.getPassphrase();
    String algoName   = view.getAlgorithm();

    if (inputPath == null || inputPath.isBlank()) {
        view.showResult("Please select an input file.");
        return;
    }
    if (passphrase == null || passphrase.isBlank()) {
        view.showResult("Please enter a passphrase or key.");
        return;
    }
    // Build request and dispatch...
}
```

This validation is not business logic (it does not know what Caesar shift-26 means), and it is not view logic (it does not touch any widget directly). It is coordination logic — the right home is the Controller.

#### Background thread execution — keeping the UI alive

Without threading, a large file encryption would freeze the JavaFX Application Thread, making the application appear hung. The Controller solves this with a JavaFX `Task`:

```java
Task<CryptoResult> task = new Task<>() {
    @Override
    protected CryptoResult call() {
        return cryptoService.execute(request);   // runs on background thread
    }
};

task.setOnSucceeded(e -> {
    CryptoResult result = task.getValue();
    view.showResult(result.getMessage());        // marshalled back to FX thread
    view.setRunning(false);
    logger.info("Result: " + result.name());
});

task.setOnFailed(e -> {
    view.showResult("Unexpected error: " + task.getException().getMessage());
    view.setRunning(false);
});

Thread t = new Thread(task);
t.setDaemon(true);   // does not prevent JVM shutdown
t.start();
```

The Controller is responsible for this threading strategy. The View does not know threads are involved. The Service does not know it is being called from a thread. Each layer has one concern.

---

### The Service Layer — `com.fileencryptor.service`

The Service layer contains all **business logic**: how to read files, how to select a cipher, how to handle in-place vs two-file operations, how to map exceptions to typed results.

#### `CryptoService.java` — Orchestrator

`CryptoService` is the engine of the application. It receives a `CryptoRequest` and returns a `CryptoResult`.

```java
public CryptoResult execute(CryptoRequest request) {
    if (request == null) return CryptoResult.ERR_NULL_PARAM;

    ICipherStrategy strategy;
    try {
        strategy = CipherStrategyFactory.create(request.getAlgorithm());
    } catch (IllegalArgumentException e) {
        return CryptoResult.ERR_UNKNOWN_ALGO;
    }

    CryptoResult keyCheck = strategy.validateKey(request.getPassphrase());
    if (keyCheck != CryptoResult.OK) return keyCheck;

    boolean inPlace = request.getInputFile().equals(request.getOutputFile());
    return inPlace
        ? executeInPlace(request, strategy)
        : executeToFile(request, strategy);
}
```

Notice: `CryptoService` never touches any JavaFX class. It never calls `Platform.runLater()`. It never reads from `MainView`. It is a pure Java class that could be unit-tested from the command line with no JavaFX runtime present.

#### In-place vs two-file strategy

The C version could open a file with `fopen(path, "rb+")` to read and write the same file. Java's `FileOutputStream` truncates the file on open, which would destroy the source data before it is read. `CryptoService` handles this transparently:

```
Two-file mode (inputFile ≠ outputFile):
  openRead(inputFile) → strategy.encrypt(in, out) → openWrite(outputFile)

In-place mode (inputFile == outputFile):
  Read ALL bytes into memory → cipher in memory → Files.write(outputFile, bytes)
```

The Controller does not choose between these modes. It simply passes both file paths and `CryptoService` detects the situation automatically.

#### Exception-to-result mapping

`CryptoService` catches all checked exceptions and maps them to typed `CryptoResult` values, so the Controller never has to handle raw `IOException` or `GeneralSecurityException`:

```java
} catch (AEADBadTagException e) {
    logger.error("AES authentication tag mismatch: " + e.getMessage());
    return CryptoResult.ERR_AUTH_FAILED;          // wrong passphrase or tampered file
} catch (IOException e) {
    logger.error("I/O error: " + e.getMessage());
    return CryptoResult.ERR_IO;
} catch (GeneralSecurityException e) {
    logger.error("Crypto error: " + e.getMessage());
    return CryptoResult.ERR_IO;
}
```

---

### The Cipher Layer — `com.fileencryptor.cipher`

The cipher layer implements the **Strategy pattern** on top of MVC. It is not strictly part of MVC itself — it is the Model's computation engine.

#### `ICipherStrategy` — the strategy interface

```java
public interface ICipherStrategy {
    CryptoResult encrypt(InputStream in, OutputStream out, String passphrase)
        throws IOException, GeneralSecurityException;

    CryptoResult decrypt(InputStream in, OutputStream out, String passphrase)
        throws IOException, GeneralSecurityException;

    CryptoResult validateKey(String passphrase);
}
```

All three algorithms — `CaesarCipherStrategy`, `XorCipherStrategy`, `AesGcmCipherStrategy` — implement this interface. `CryptoService` interacts with all three through only this interface and never uses `instanceof` or type casts.

#### `CipherStrategyFactory` — algorithm selection

```java
public class CipherStrategyFactory {
    public static ICipherStrategy create(Algorithm algo) {
        return switch (algo) {
            case CAESAR_CIPHER -> new CaesarCipherStrategy();
            case XOR_CIPHER    -> new XorCipherStrategy();
            case AES_256_GCM   -> new AesGcmCipherStrategy();
        };
    }
}
```

The factory decouples algorithm selection from instantiation. Adding a new algorithm (e.g. ChaCha20) means adding one `case` here — `CryptoService`, `MainController`, and `MainView` remain untouched.

---

### The Composition Root — `App.java`

`App.java` is the **only** class in the entire project that imports concrete implementation classes. Everything else depends only on interfaces or sibling-layer types:

```java
public class App extends Application {
    @Override
    public void start(Stage primaryStage) {
        // Infrastructure
        ILogger      logger      = new FileLogger("file_encryption.log");
        IFileHandler fileHandler = new FileHandlerImpl();

        // Service (depends on interfaces, not concrete classes)
        CryptoService cryptoService = new CryptoService(fileHandler, logger);

        // View
        MainView view = new MainView(primaryStage);

        // Controller (wires view to service)
        MainController controller = new MainController(view, cryptoService, logger);

        primaryStage.show();
        logger.info("Application window displayed.");
    }
}
```

This is **dependency injection by hand** (no framework). The composition root assembles the full object graph and injects dependencies downward. This single wiring point is the reason the entire rest of the codebase can be tested with mocks — you just swap the injected implementations in the constructor call.

---

### MVC Communication Summary

```
┌──────────────────────────────────────────────────────────────────────────┐
│                        Communication Contracts                           │
├───────────────────────┬──────────────────────────────────────────────────┤
│ View  → Controller    │ Runnable / Consumer<T> callbacks registered by   │
│                       │ Controller at startup. View fires; Controller    │
│                       │ decides. No direct method calls from View.       │
├───────────────────────┼──────────────────────────────────────────────────┤
│ Controller → View     │ Direct method calls on MainView's write API:     │
│                       │ showResult(), showStatus(), setRunning(), etc.   │
│                       │ All safe to call from Task.setOnSucceeded (FX    │
│                       │ Application Thread guaranteed).                  │
├───────────────────────┼──────────────────────────────────────────────────┤
│ Controller → Service  │ cryptoService.execute(CryptoRequest) called from │
│                       │ a JavaFX Task on a daemon background thread.     │
├───────────────────────┼──────────────────────────────────────────────────┤
│ Service → Cipher      │ strategy.encrypt() / strategy.decrypt() called   │
│                       │ through ICipherStrategy interface — no concrete  │
│                       │ cipher class is ever referenced by the service.  │
├───────────────────────┼──────────────────────────────────────────────────┤
│ Service → Infra       │ fileHandler.openRead() / openWrite() and         │
│                       │ logger.info() / warn() / error() called through  │
│                       │ IFileHandler and ILogger interfaces.             │
└───────────────────────┴──────────────────────────────────────────────────┘
```

---

### Why MVC? Benefits in This Project

| Benefit | How it appears here |
|---|---|
| **Independent changeability** | The dark Catppuccin CSS theme (`style.css`) was changed multiple times without touching any Java class. Similarly, the AES file format diverged from the C version without any View changes. |
| **Testability** | `CryptoService` can be unit-tested by passing a mock `IFileHandler` (in-memory byte arrays) and a mock `ILogger` (no-op). No JavaFX runtime is needed. |
| **Parallel development** | One developer can work on `MainView` layout while another implements `AesGcmCipherStrategy` — the only shared contract is the interface and the read/write API. |
| **UI responsiveness** | The threading model (JavaFX `Task` in Controller) keeps the View responsive during large-file operations — the Model/Service code required zero changes to support this. |
| **Scalability** | Adding a new screen (e.g. a settings panel) means adding a new View + Controller pair. The existing Service layer is reused untouched. |

---

## Project Structure

```
FileEncryptorJava/
├── pom.xml                                        ← Maven build (replaces Makefile)
├── build-installer.ps1                            ← PowerShell script for .exe installer
├── icon.ico                                       ← App icon for installer
├── file_encryption.log                            ← Created on first run
├── README.md
└── src/
    └── main/
        ├── java/
        │   ├── module-info.java                   ← Java 9 module descriptor
        │   └── com/fileencryptor/
        │       │
        │       ├── App.java                       ← Entry point + DI composition root
        │       │
        │       ├── model/                         ← Pure data, zero dependencies
        │       │   ├── Algorithm.java             ← Enum: CAESAR, XOR, AES_256_GCM
        │       │   ├── CryptoRequest.java         ← Immutable value object (inputs)
        │       │   └── CryptoResult.java          ← Enum: OK, ERR_INVALID_KEY, etc.
        │       │
        │       ├── cipher/                        ← Cipher strategy pattern
        │       │   ├── ICipherStrategy.java       ← Interface: encrypt/decrypt/validate
        │       │   ├── CaesarCipherStrategy.java  ← Shift alphabetic bytes by key
        │       │   ├── XorCipherStrategy.java     ← XOR every byte with key
        │       │   ├── AesGcmCipherStrategy.java  ← AES-256-GCM via JCA
        │       │   └── CipherStrategyFactory.java ← Maps Algorithm → ICipherStrategy
        │       │
        │       ├── service/                       ← Business logic layer
        │       │   ├── IFileHandler.java          ← Interface: openRead/openWrite/exists
        │       │   ├── FileHandlerImpl.java       ← Buffered java.io implementation
        │       │   ├── ILogger.java               ← Interface: info/warn/error
        │       │   └── CryptoService.java         ← Orchestrator: I/O + cipher
        │       │
        │       ├── controller/                    ← MVC Controller
        │       │   └── MainController.java        ← Wires view events to service calls
        │       │
        │       ├── view/                          ← MVC View
        │       │   └── MainView.java              ← All JavaFX widgets + layout
        │       │
        │       └── util/                          ← Cross-cutting utilities
        │           └── FileLogger.java            ← Timestamped file + stderr logger
        │
        └── resources/
            └── com/fileencryptor/css/
                └── style.css                      ← Dark Catppuccin theme
```

---

## Algorithms

### Caesar Cipher
A classical substitution cipher that shifts each alphabetic character by a fixed key. Non-alphabetic bytes (digits, punctuation, binary data) pass through unchanged — making it safe for any file type.

- **Key range:** 1 – 25
- **Java class:** `CaesarCipherStrategy`
- **Decrypt:** inverse shift = `(26 - key)`
- **Use case:** Educational / demonstration only. Not secure for real data.

### XOR Cipher
Every byte of the file is XOR-ed with the key byte. XOR is its own inverse — the same operation both encrypts and decrypts. Works on any binary content.

- **Key range:** 1 – 255
- **Java class:** `XorCipherStrategy`
- **Use case:** Lightweight obfuscation. Not cryptographically secure on its own.

### AES-256-GCM *(primary secure algorithm)*
Industry-standard symmetric authenticated encryption using Java's built-in `javax.crypto` package. A 256-bit key is derived from the user's passphrase via SHA-256 (matching the C project's `aes_gcm_derive_key`). GCM mode provides both confidentiality and integrity — a wrong key or tampered file is detected and rejected before any output is written.

- **Key derivation:** `SHA-256(passphrase)` → 32-byte AES key
- **IV:** 12 bytes, randomly generated per encryption via `SecureRandom`
- **Authentication tag:** 16 bytes (appended by JCA after ciphertext)
- **Output format:** `[ 12B IV ][ AES-GCM ciphertext + 16B tag ]`
- **Java class:** `AesGcmCipherStrategy`

---

## SOLID Principles Applied

### S — Single Responsibility Principle
> *"A class should have only one reason to change."*

Every class in the project has exactly one job:

| Class | Single responsibility |
|---|---|
| `MainView` | Build and display JavaFX widgets; expose a read/write API |
| `MainController` | Collect UI inputs; coordinate between view and service |
| `CryptoService` | Orchestrate one encrypt/decrypt operation end-to-end |
| `CaesarCipherStrategy` | Implement Caesar cipher logic only |
| `XorCipherStrategy` | Implement XOR cipher logic only |
| `AesGcmCipherStrategy` | Implement AES-256-GCM logic only |
| `FileHandlerImpl` | Open and close physical files |
| `FileLogger` | Write timestamped log entries to file and stderr |
| `CipherStrategyFactory` | Map an `Algorithm` enum to an `ICipherStrategy` instance |
| `CryptoRequest` | Hold all inputs for one operation as an immutable value object |
| `CryptoResult` | Model all possible operation outcomes and their messages |

**Example:** `AesGcmCipherStrategy` knows nothing about files, UI, or logging. If the AES implementation needs to change, only this one class is touched.

---

### O — Open/Closed Principle
> *"Classes should be open for extension but closed for modification."*

Adding a new encryption algorithm (e.g. ChaCha20) requires:
1. Add a new constant to `Algorithm.java` enum
2. Create one new class implementing `ICipherStrategy`
3. Add one `case` in `CipherStrategyFactory`

**Zero existing classes are modified.** `CryptoService`, `MainController`, `MainView` — all remain untouched.

```
Algorithm enum          +──── CHACHA20 (new constant)
ICipherStrategy         ←──── ChaCha20Strategy implements (new class)
CipherStrategyFactory   +──── case CHACHA20: return new ChaCha20Strategy()
─────────────────────────────────────────────────────────────────
CryptoService           ← unchanged
MainController          ← unchanged
MainView                ← unchanged
```

---

### L — Liskov Substitution Principle
> *"Objects of a subclass should be replaceable with objects of the superclass without breaking the application."*

All three cipher strategies (`CaesarCipherStrategy`, `XorCipherStrategy`, `AesGcmCipherStrategy`) implement `ICipherStrategy`. `CryptoService` works with any of them through the interface alone:

```java
ICipherStrategy strategy = CipherStrategyFactory.create(request.getAlgorithm());
strategy.encrypt(in, out, passphrase);  // works for all three — LSP satisfied
```

Similarly, `FileHandlerImpl` implements `IFileHandler`. Any alternative implementation (e.g. an in-memory mock for testing) substitutes in without changing `CryptoService`.

---

### I — Interface Segregation Principle
> *"Clients should not be forced to depend on interfaces they do not use."*

Each interface exposes only the minimal set of methods its consumers actually need:

| Interface | Methods | Why narrow |
|---|---|---|
| `ICipherStrategy` | `encrypt`, `decrypt`, `validateKey` | Cipher callers need exactly these three |
| `IFileHandler` | `openRead`, `openWrite`, `exists`, `size` | Service needs exactly these four |
| `ILogger` | `info`, `warn`, `error` | Application only uses three log levels |

No fat interfaces exist. No class is forced to implement methods it does not use.

---

### D — Dependency Inversion Principle
> *"High-level modules should not depend on low-level modules. Both should depend on abstractions."*

`CryptoService` (high-level) depends on `IFileHandler` and `ILogger` (interfaces) — never on `FileHandlerImpl` or `FileLogger` (concrete classes):

```java
// CryptoService constructor — depends on interfaces only
public CryptoService(IFileHandler fileHandler, ILogger logger) {
    this.fileHandler = fileHandler;
    this.logger      = logger;
}
```

All concrete wiring happens in `App.java` (the composition root) — the only class that references implementations:

```java
// App.java — the ONLY place that touches concrete classes
FileLogger      logger        = new FileLogger("file_encryption.log");
FileHandlerImpl fileHandler   = new FileHandlerImpl();
CryptoService   cryptoService = new CryptoService(fileHandler, logger);
MainView        view          = new MainView(primaryStage);
MainController  controller    = new MainController(view, cryptoService, logger);
```

This means `CryptoService` can be tested with mock implementations of `IFileHandler` and `ILogger` without touching any real files or producing any real log output.

---

## Design Patterns

| Pattern | Classes | Purpose |
|---|---|---|
| **Strategy** | `ICipherStrategy` + 3 implementations | Swap cipher algorithms at runtime without conditionals in the service |
| **Factory** | `CipherStrategyFactory` | Decouple algorithm selection from cipher construction |
| **MVC** | `MainView` / `MainController` / `CryptoService` + model | Separate UI, coordination, and business logic |
| **Value Object** | `CryptoRequest` | Immutable bundle of all inputs — safe to pass across threads |
| **Observer / Callback** | `setOnBrowseInput`, `setOnRun`, etc. in `MainView` | View fires events; controller registers handlers — no tight coupling |
| **Template Method** | `CryptoService.execute()` | Fixed algorithm skeleton (validate → open → cipher → close) with variable cipher step |

---

## UML Class Diagram

```
<<enumeration>>           <<enumeration>>          <<value object>>
Algorithm                 CryptoResult             CryptoRequest
─────────────────         ──────────────────       ──────────────────────
CAESAR_CIPHER             OK                       -inputFile  : File
XOR_CIPHER                ERR_INVALID_KEY          -outputFile : File
AES_256_GCM               ERR_UNKNOWN_ALGO         -algorithm  : Algorithm
+getDisplayName()         ERR_IO                   -passphrase : String
+getKeyHint()             ERR_AUTH_FAILED          -encrypt    : boolean
+fromDisplayName()        ERR_NULL_PARAM           +getters...
                          +getMessage()

──────────────────────────────────────────────────────────────────────────

<<interface>>                    <<interface>>           <<interface>>
ICipherStrategy                  IFileHandler             ILogger
────────────────────             ───────────────          ───────────────
+encrypt(in,out,pass)            +openRead(File)          +info(msg)
+decrypt(in,out,pass)            +openWrite(File)         +warn(msg)
+validateKey(pass)               +exists(File)            +error(msg)
        ▲                        +size(File)                    ▲
        │ implements                     ▲ implements            │ implements
┌───────┴──────────────┐      ┌─────────┴──────────┐   ┌───────┴─────────┐
│ CaesarCipherStrategy │      │ FileHandlerImpl     │   │ FileLogger      │
│ XorCipherStrategy    │      └────────────────────┘   └─────────────────┘
│ AesGcmCipherStrategy │
└──────────────────────┘

──────────────────────────────────────────────────────────────────────────

CipherStrategyFactory              CryptoService
──────────────────────             ────────────────────────────────────
+create(Algorithm)                 -fileHandler : IFileHandler
  : ICipherStrategy                -logger      : ILogger
                                   +execute(CryptoRequest) : CryptoResult
                                   -executeToFile(...)
                                   -executeInPlace(...)

──────────────────────────────────────────────────────────────────────────

MainView                           MainController
────────────────────────           ──────────────────────────────────────
-stage : Stage                     -view          : MainView
-algoCombo : ComboBox              -cryptoService : CryptoService
-encryptRadio : RadioButton        -logger        : ILogger
-passphraseField : PasswordField   -selectedInputFile  : File
-statusLabel : Label               -selectedOutputFile : File
-resultLabel : Label               +handleBrowseInput()
+setOnBrowseInput(Runnable)        +handleBrowseOutput()
+setOnRun(Runnable)                +handleRun()
+setOnClear(Runnable)              +handleClear()
+getPassphrase() : String          +handleAlgorithmChange(String)
+isEncryptSelected() : boolean
+reset()
+setRunning(boolean)

──────────────────────────────────────────────────────────────────────────

App  (composition root)
────────────────────────────────────────────────────────────────
creates: FileLogger → FileHandlerImpl → CryptoService
creates: MainView → MainController
(only class that references concrete implementations)
```

---

## C vs Java — Full Comparison

### Language & Tooling

| Aspect | C Version | Java Version |
|---|---|---|
| Language | C99 | Java 21 |
| GUI framework | GTK4 | JavaFX 21 |
| Crypto library | OpenSSL EVP | JCA (`javax.crypto`) |
| Build tool | Make (`Makefile`) | Maven (`pom.xml`) |
| Compile command | `make` | `mvn compile` |
| Run command | `./file_encryptor.exe` | `mvn javafx:run` |
| Clean command | `make clean` | `mvn clean` |
| Packaging | Manual GCC linking | `mvn package` → `.jar` |
| Installer | None | `jpackage` → `.exe` |

---

### Architecture Comparison

| Aspect | C Version | Java Version |
|---|---|---|
| Architecture | Layered (4 layers, procedural) | MVC + Layered (4 layers, OOP) |
| State management | `AppState` struct passed through context structs | `CryptoRequest` immutable value object |
| Coupling | Low (via context structs) | Very low (via interfaces) |
| Testability | Difficult (no dependency injection) | Easy (interfaces injectable with mocks) |
| Adding new algorithm | Add `.c` + `.h` + `if` in dispatcher | Add enum constant + implement `ICipherStrategy` |

---

### Module-by-Module Mapping

| C Module | Java Class(es) | Notes |
|---|---|---|
| `main.c` | `App.java` | Entry point + composition root |
| `ui.c` | `MainView.java` + `MainController.java` | Split into View and Controller |
| `crypto_dispatcher.c` | `CryptoService.java` + `CipherStrategyFactory.java` | Dispatcher split into service + factory |
| `caesar_cipher.c` | `CaesarCipherStrategy.java` | Same algorithm, Java streams |
| `xor_cipher.c` | `XorCipherStrategy.java` | Same algorithm, Java streams |
| `aes_gcm.c` | `AesGcmCipherStrategy.java` | OpenSSL EVP → JCA `javax.crypto` |
| `file_handler.c` | `FileHandlerImpl.java` | `fopen/fclose` → `BufferedInputStream` |
| `logger.c` | `FileLogger.java` | Same format, Java `LocalDateTime` |
| `common.h` constants | `Algorithm.java` + `CryptoResult.java` | Enums instead of `#define` + `typedef enum` |
| `AppState` struct | `CryptoRequest.java` | Immutable value object instead of mutable struct |

---

### Key Technical Differences

| Feature | C Version | Java Version | Reason for change |
|---|---|---|---|
| GUI event handling | GTK signal callbacks (`g_signal_connect`) | JavaFX lambdas via `setOn*` callbacks | Java idiomatic event handling |
| File I/O | `fopen` / `fread` / `fwrite` | `BufferedInputStream` / `BufferedOutputStream` | Java standard library |
| AES crypto API | OpenSSL `EVP_EncryptInit_ex` | JCA `Cipher.getInstance("AES/GCM/NoPadding")` | Java built-in, no external lib |
| Random IV | `RAND_bytes(iv, 12)` | `new SecureRandom().nextBytes(iv)` | Equivalent CSPRNG |
| SHA-256 key derivation | `SHA256(passphrase, len, key)` | `MessageDigest.getInstance("SHA-256")` | Equivalent output |
| File format | `[12B IV][16B TAG][ciphertext]` | `[12B IV][ciphertext + 16B TAG]` | JCA appends tag after ciphertext |
| In-place file handling | `fopen(path, "rb+")` — no truncation | Read-all → cipher in memory → write back | Java `FileOutputStream` truncates on open |
| Same-file bug | Not a bug in C (`rb+` mode) | Fixed with read-all-then-write-back | Java stream semantics differ from C |
| Background execution | Blocking (UI freezes on large files) | JavaFX `Task` on daemon thread | UI always stays responsive |
| Log path | Working directory only | Smart resolution: working dir → user home | Handles `C:\Program Files` write restriction |
| Dependency management | Manual (`apt install libgtk-4-dev`) | Automatic (`mvn` downloads JavaFX) | Maven downloads all dependencies |

---

### Build Command Comparison

```
C Project                          Java Project
─────────────────────────          ─────────────────────────
make                         ←→    mvn compile
make clean && make           ←→    mvn clean compile
./file_encryptor.exe         ←→    mvn javafx:run
make clean && ./...          ←→    mvn clean javafx:run
(no equivalent)              ←→    mvn package  (→ .jar)
(no equivalent)              ←→    jpackage     (→ .exe installer)
```

---

## Dependencies & Requirements

### To Run the App (Development)

| Requirement | Minimum Version | Download |
|---|---|---|
| Java JDK | 17+ (21 recommended) | https://adoptium.net |
| Apache Maven | 3.8+ | https://maven.apache.org/download.cgi |

JavaFX is **downloaded automatically by Maven** — no manual installation needed.

### To Build the Windows Installer

| Requirement | Version | Download |
|---|---|---|
| Java JDK | 17+ (must include `jpackage`) | https://adoptium.net |
| WiX Toolset | 3.14 | https://github.com/wixtoolset/wix3/releases |
| JavaFX jmods | 21.0.10 Windows x64 | https://gluonhq.com/products/javafx/ |

---

## Building & Running

### Quick Start

```bash
# 1. Clone or extract the project
cd FileEncryptorJava

# 2. Run directly (Maven downloads JavaFX automatically on first run)
mvn javafx:run
```

### All Maven Commands

```bash
# Run the application
mvn javafx:run

# Clean old build files (like make clean)
mvn clean

# Compile only — check for errors without running
mvn compile

# Clean then run fresh (most reliable)
mvn clean javafx:run

# Build distributable JAR
mvn clean package
java -jar target/file-encryptor.jar
```

### Maven Build Lifecycle

```
mvn clean     → deletes target/ folder
mvn compile   → compiles all .java → .class files
mvn package   → compile + package into .jar
mvn javafx:run → compile + run with JavaFX module path
```

### First Run Note

On the very first `mvn javafx:run`, Maven downloads JavaFX (~50 MB) into `~/.m2/repository/`. Subsequent runs are instant — it never downloads again.

### Using an IDE

| IDE | How to open and run |
|---|---|
| **IntelliJ IDEA** (recommended) | File → Open → select `FileEncryptorJava` folder → Maven auto-detected → right-click `App.java` → Run |
| **Eclipse** | File → Import → Existing Maven Projects → browse to folder → run `App.java` |
| **VS Code** | Install "Extension Pack for Java" → Open Folder → run `App.java` |

---

## Creating a Windows Installer (.exe)

### Step 1 — Install WiX Toolset

1. Go to https://github.com/wixtoolset/wix3/releases
2. Download **`wix314.exe`** and install it
3. Add `C:\Program Files (x86)\WiX Toolset v3.14\bin` to system PATH
4. Verify: open new PowerShell → `candle --version`

### Step 2 — Download JavaFX jmods

1. Go to https://gluonhq.com/products/javafx/
2. Select: Version **21.0.10** · OS **Windows** · Arch **x64** · Type **jmods**
3. Download and extract to `C:\javafx-jmods-21.0.10`
4. Verify: `C:\javafx-jmods-21.0.10\javafx.controls.jmod` exists

### Step 3 — Run the Build Script

```powershell
cd "F:\Path\To\FileEncryptorJava"
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\build-installer.ps1
```

The script automatically:
- Checks all prerequisites
- Runs `mvn clean package`
- Runs `jpackage` with all required flags
- Reports the installer location

### Step 4 — Install

```
installer\FileEncryptor-1.0.0.exe   ← double-click to install
```

Windows will show a UAC prompt (Allow changes?) — click **Yes**.

The installer creates:
```
C:\Program Files\FileEncryptor\
    FileEncryptor.exe     ← launcher (double-clickable)
    app\
        file-encryptor.jar
        libs\             ← JavaFX JARs
    runtime\
        bin\java.exe      ← bundled Java (user needs nothing installed)
```

After installation the app appears in **Start Menu** and on the **Desktop**.

---

## Usage Guide

1. **Select Input File** — Click *Browse…* next to "Input File" and choose the file to encrypt or decrypt.
2. **Select Output File** — Click *Browse…* next to "Output File". To encrypt/decrypt in-place, select the same file as the input.
3. **Choose Algorithm** — Select from the dropdown: *Caesar Cipher*, *XOR Cipher*, or *AES-256-GCM*.
4. **Select Action** — Choose *Encrypt* or *Decrypt* using the radio buttons.
5. **Enter Key or Passphrase:**
   - Caesar Cipher: whole number 1 – 25
   - XOR Cipher: whole number 1 – 255
   - AES-256-GCM: any passphrase string (longer = stronger)
6. **Click Run** — The operation runs on a background thread. The status label shows progress.
7. **Read Result** — The result label shows success or a specific error message.
8. **Clear** — Resets all fields to their initial state.

> **⚠️ Important:** AES-256-GCM encrypted files can only be decrypted with the exact same passphrase. There is no recovery mechanism. Store your passphrase securely.

---

## Log File Location

The log file `file_encryption.log` is written with timestamped INFO / WARN / ERROR entries. Its location depends on how the app is running:

| How app is running | Log file location |
|---|---|
| `mvn javafx:run` | Project root folder: `FileEncryptorJava\file_encryption.log` |
| Installed `.exe` (Program Files) | `C:\Users\<username>\FileEncryptor\file_encryption.log` |

The app automatically detects whether it can write to the current working directory. If not (e.g. `C:\Program Files\` is read-only), it falls back to `C:\Users\<username>\FileEncryptor\` which is always writable.

**Open the log from PowerShell:**
```powershell
notepad "$env:USERPROFILE\FileEncryptor\file_encryption.log"
```

**Sample log output:**
```
[2026-03-25 16:14:01] [INFO ] === File Encryption Application starting (JavaFX) ===
[2026-03-25 16:14:01] [INFO ] Algorithms available: Caesar Cipher, XOR Cipher, AES-256-GCM
[2026-03-25 16:14:03] [INFO ] Application window displayed.
[2026-03-25 16:14:22] [INFO ] Starting encryption with AES-256-GCM  [report.pdf → report.pdf] (in-place)
[2026-03-25 16:14:22] [INFO ] In-place: read 204800 bytes into memory.
[2026-03-25 16:14:22] [INFO ] In-place: wrote 204829 bytes back to file.
[2026-03-25 16:14:22] [INFO ] Operation completed successfully.
```

---

## Security Design

### Encrypted File Format (AES-256-GCM)

```
┌──────────────────────────────┐
│   12 bytes   IV              │  Random IV — unique per encryption (SecureRandom)
├──────────────────────────────┤
│   N  bytes   Ciphertext      │  AES-256-GCM encrypted content
├──────────────────────────────┤
│   16 bytes   GCM Tag         │  Authentication tag — appended by JCA after ciphertext
└──────────────────────────────┘
```

> **Note on format difference from C version:**
> The C version stores `[IV][TAG][Ciphertext]` (tag before ciphertext).
> The Java JCA stores `[IV][Ciphertext+TAG]` (tag appended at end).
> Files encrypted by the C version cannot be decrypted by the Java version and vice versa, because the tag position differs.

### Key Derivation

```
passphrase ──→ SHA-256 ──→ 256-bit AES key
```

Matches the C implementation's `aes_gcm_derive_key()` function exactly.

### Threat Model Coverage

| Threat | Mitigation |
|---|---|
| Ciphertext-only analysis | AES-256-GCM (IND-CCA2 secure) |
| Identical-plaintext pattern leakage | Unique random IV per encryption via `SecureRandom` |
| File tampering / corruption | GCM tag verified by JCA before writing any plaintext output |
| IV reuse | `SecureRandom.nextBytes()` — cryptographically secure |
| Stale bytes after in-place decrypt | Read-all-then-write-back strategy; `Files.write()` replaces file atomically |
| Weak passphrase brute force | Not mitigated — SHA-256 with no salt or iterations (future: PBKDF2) |

---

## Known Limitations

1. **Key derivation** uses a single SHA-256 pass with no salt and no iterations. A future improvement would use PBKDF2-HMAC-SHA256 with a random salt and 100,000+ iterations to resist brute-force attacks on weak passphrases.
2. **AES file format incompatibility** with the C version — tag is stored in a different position.
3. **In-place encryption** loads the entire file into memory. For very large files (>500 MB) a temp-file strategy would be more memory-efficient.
4. **No progress bar** for large file operations (though the UI does not freeze due to background threading).
5. **Caesar and XOR ciphers** are not cryptographically secure — included for educational purposes only.
6. **No automated test suite** yet.
7. **Windows SmartScreen warning** on the `.exe` installer — the binary is not code-signed.

---

## Future Improvements

- [ ] Upgrade key derivation from SHA-256 to PBKDF2-HMAC-SHA256 with random salt
- [ ] Add RSA public-key encryption for asymmetric key exchange
- [ ] Progress bar for large file operations (JavaFX `ProgressBar` bound to `Task.progressProperty()`)
- [ ] File drag-and-drop support (JavaFX `setOnDragDropped`)
- [ ] Unit tests with JUnit 5 + Mockito (mock `IFileHandler` / `ILogger`)
- [ ] FXML-based UI layout instead of programmatic JavaFX
- [ ] Unify AES file format with the C version
- [ ] Multi-file batch encryption
- [ ] Code signing certificate for `.exe` installer (removes SmartScreen warning)
- [ ] Cloud storage integration (upload encrypted output to Google Drive / OneDrive)
- [ ] Post-quantum cryptography readiness (CRYSTALS-Kyber via BouncyCastle)

---

## Resource Links

### Core Technologies

| Resource | URL |
|---|---|
| Java 21 JDK (Adoptium Temurin) | https://adoptium.net |
| JavaFX 21 Documentation | https://openjfx.io |
| JavaFX API Reference | https://openjfx.io/javadoc/21/ |
| Apache Maven Download | https://maven.apache.org/download.cgi |
| Maven Getting Started Guide | https://maven.apache.org/guides/getting-started/ |

### JavaFX Learning

| Resource | URL |
|---|---|
| JavaFX Getting Started (Oracle) | https://docs.oracle.com/javafx/2/get_started/jfxpub-get_started.htm |
| OpenJFX Getting Started | https://openjfx.io/openjfx-docs/ |
| JavaFX Tutorial — jenkov.com | https://jenkov.com/tutorials/javafx/index.html |
| JavaFX Maven Plugin | https://github.com/openjfx/javafx-maven-plugin |

### Cryptography

| Resource | URL |
|---|---|
| Java Cryptography Architecture (JCA) Guide | https://docs.oracle.com/en/java/javase/21/security/java-cryptography-architecture-jca-reference-guide.html |
| AES-GCM explained | https://en.wikipedia.org/wiki/Galois/Counter_Mode |
| SecureRandom Javadoc | https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/security/SecureRandom.html |
| javax.crypto.Cipher Javadoc | https://docs.oracle.com/en/java/javase/21/docs/api/java.base/javax/crypto/Cipher.html |

### SOLID Principles

| Resource | URL |
|---|---|
| SOLID Principles (Wikipedia) | https://en.wikipedia.org/wiki/SOLID |
| SOLID in Java (Baeldung) | https://www.baeldung.com/solid-principles |
| Strategy Pattern (Refactoring.Guru) | https://refactoring.guru/design-patterns/strategy |
| Factory Method Pattern | https://refactoring.guru/design-patterns/factory-method |
| MVC Pattern explained | https://www.geeksforgeeks.org/mvc-design-pattern/ |

### Windows Installer

| Resource | URL |
|---|---|
| jpackage official documentation | https://docs.oracle.com/en/java/javase/21/docs/specs/man/jpackage.html |
| WiX Toolset 3.14 releases | https://github.com/wixtoolset/wix3/releases |
| JavaFX jmods download (Gluon) | https://gluonhq.com/products/javafx/ |
| JavaFX packaging guide | https://openjfx.io/openjfx-docs/#packaging |

### Tools & IDEs

| Resource | URL |
|---|---|
| IntelliJ IDEA Community (free) | https://www.jetbrains.com/idea/download/ |
| VS Code Java Extension Pack | https://marketplace.visualstudio.com/items?itemName=vscjava.vscode-java-pack |
| Eclipse IDE for Java | https://www.eclipse.org/downloads/ |
| Maven Repository (find JARs) | https://mvnrepository.com |

---

*File Encryptor Java/JavaFX Edition — Advanced Programming Laboratory · 2026*
