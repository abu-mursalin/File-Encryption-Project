Write-Host ""
Write-Host "==== File Encryptor - Installer Build Script ====" -ForegroundColor Cyan
Write-Host ""

$JAVAFX_JMODS = "C:\javafx-jmods-21.0.10"

Write-Host "[1/4] Checking prerequisites..." -ForegroundColor Yellow

if (-not (Test-Path $JAVAFX_JMODS)) {
    Write-Host "ERROR: JavaFX jmods not found at $JAVAFX_JMODS" -ForegroundColor Red
    Write-Host "Download from https://gluonhq.com/products/javafx/ and extract to C:\javafx-jmods-21.0.10" -ForegroundColor White
    exit 1
}
Write-Host "  JavaFX jmods: OK" -ForegroundColor Green

$jp = Get-Command jpackage -ErrorAction SilentlyContinue
if (-not $jp) {
    Write-Host "ERROR: jpackage not found on PATH" -ForegroundColor Red
    exit 1
}
Write-Host "  jpackage: OK" -ForegroundColor Green

$wix = Get-Command candle -ErrorAction SilentlyContinue
if (-not $wix) {
    Write-Host "ERROR: WiX candle not found on PATH" -ForegroundColor Red
    exit 1
}
Write-Host "  WiX: OK" -ForegroundColor Green
Write-Host ""

Write-Host "[2/4] Running Maven build..." -ForegroundColor Yellow
mvn clean package -q
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Maven build failed. Run mvn clean package to see details." -ForegroundColor Red
    exit 1
}
Write-Host "  Maven build: SUCCESS" -ForegroundColor Green
Write-Host ""

Write-Host "[3/4] Preparing output folder..." -ForegroundColor Yellow
if (Test-Path "installer") {
    Remove-Item -Recurse -Force "installer"
}
New-Item -ItemType Directory -Path "installer" | Out-Null
Write-Host "  installer folder ready" -ForegroundColor Green
Write-Host ""

Write-Host "[4/4] Running jpackage..." -ForegroundColor Yellow
Write-Host "  App will install to: C:\Program Files\FileEncryptor\" -ForegroundColor Green
Write-Host "  Log file will save to: C:\Users\$env:USERNAME\FileEncryptor\file_encryption.log" -ForegroundColor Green
Write-Host ""

# No --install-dir and no --win-per-user-install
# so jpackage defaults to C:\Program Files\FileEncryptor\
# Requires Administrator during install — Windows will ask automatically
$jpackageArgs = @(
    "--input",               "target",
    "--main-jar",            "file-encryptor.jar",
    "--main-class",          "com.fileencryptor.App",
    "--name",                "FileEncryptor",
    "--app-version",         "1.0.0",
    "--vendor",              "Mursalin and Udoy",
    "--description",         "File Encryption Tool",
    "--type",                "exe",
    "--win-shortcut",
    "--win-menu",
    "--win-menu-group",      "FileEncryptor",
    "--win-dir-chooser",
    "--module-path",         $JAVAFX_JMODS,
    "--add-modules",         "javafx.controls,javafx.fxml,javafx.graphics,javafx.base",
    "--dest",                "installer",
    "--java-options",        "--enable-native-access=javafx.graphics"
)

if (Test-Path "icon.ico") {
    $jpackageArgs += "--icon"
    $jpackageArgs += "icon.ico"
    Write-Host "  Using icon.ico" -ForegroundColor Green
}

& jpackage @jpackageArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: jpackage failed. See output above." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "==== BUILD SUCCESSFUL ====" -ForegroundColor Green
Write-Host ""
Write-Host "Installer location:" -ForegroundColor White
Get-ChildItem "installer\*.exe" | ForEach-Object { Write-Host "  $($_.FullName)" -ForegroundColor Cyan }
Write-Host ""
Write-Host "After installing:" -ForegroundColor White
Write-Host "  App installed at  : C:\Program Files\FileEncryptor\" -ForegroundColor Cyan
Write-Host "  Log file saved at : C:\Users\$env:USERNAME\FileEncryptor\file_encryption.log" -ForegroundColor Cyan
Write-Host ""
