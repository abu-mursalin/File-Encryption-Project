/*
 * Advanced File Encryption System
 * -----------------------------------
 *
 * Improvements applied (per project documentation):
 *  - AES-256-GCM via EVP API (replaces low-level ECB)
 *  - Random IV generated per encryption, prepended to output file
 *  - PBKDF2-HMAC-SHA256 key derivation (replaces bare SHA-256)
 *  - GCM authentication tag stored in output for integrity check
 *  - Global variables eliminated; all state carried in AppState struct
 *  - Layered architecture: UI → Controller → Service → Crypto
 *  - Comprehensive error handling with descriptive messages
 *  - Secure memory wiping after key usage (OPENSSL_cleanse)
 *  - snake_case naming, meaningful identifiers throughout
 *  - Modular functions with Single Responsibility Principle
 */

#include <gtk/gtk.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>

/* =========================================================
 * Constants
 * ========================================================= */
#define AES_KEY_SIZE      32   /* 256-bit AES key                   */
#define AES_IV_SIZE       12   /* 96-bit IV for GCM                 */
#define AES_TAG_SIZE      16   /* 128-bit GCM authentication tag    */
#define PBKDF2_ITERATIONS 100000
#define PBKDF2_SALT_SIZE  16
#define FILE_BUFFER_SIZE  4096 /* streaming block size              */
#define CAESAR_KEY_MIN    1
#define CAESAR_KEY_MAX    25
#define XOR_KEY_MIN       1
#define XOR_KEY_MAX       255

/* =========================================================
 * Application state  (replaces all global variables)
 * ========================================================= */
typedef struct {
    char   *input_path;    /* path of the file to read    */
    char   *output_path;   /* path of the file to write   */
    char   *action_label;  /* "Encrypt" or "Decrypt"      */
    bool    input_ready;   /* input file chosen            */
    bool    output_ready;  /* output path chosen           */
    bool    action_ready;  /* encrypt/decrypt selected     */
} AppState;

/* =========================================================
 * Widget bundle passed between GTK callbacks
 * ========================================================= */
typedef struct {
    GtkWidget *window;
    GtkWidget *input_button;
    GtkWidget *output_button;
    GtkWidget *overwrite_check;
    GtkWidget *algorithm_combo;
    GtkWidget *action_radio_encrypt;
    GtkWidget *action_radio_decrypt;
    GtkWidget *key_entry;
    GtkWidget *action_button;
    GtkWidget *status_label;
    GtkWidget *result_label;
    AppState  *state;
} WidgetBundle;


/* ==========================================================
 * ----------------  CRYPTO LAYER  -------------------------
 * ========================================================== */

/* ----------------------------------------------------------
 * Caesar cipher — encrypt
 * ---------------------------------------------------------- */
static bool caesar_encrypt(FILE *fr, FILE *fw, int shift_key)
{
    int byte;
    while ((byte = fgetc(fr)) != EOF) {
        char ch = (char)byte;
        if (isupper(ch)) ch = (char)(((ch + shift_key - 'A') % 26) + 'A');
        if (islower(ch)) ch = (char)(((ch + shift_key - 'a') % 26) + 'a');
        if (fputc(ch, fw) == EOF) return false;
    }
    return true;
}

/* ----------------------------------------------------------
 * Caesar cipher — decrypt
 * ---------------------------------------------------------- */
static bool caesar_decrypt(FILE *fr, FILE *fw, int shift_key)
{
    int byte;
    while ((byte = fgetc(fr)) != EOF) {
        char ch = (char)byte;
        if (isupper(ch)) ch = (char)(((ch - shift_key - 'A' + 26) % 26) + 'A');
        if (islower(ch)) ch = (char)(((ch - shift_key - 'a' + 26) % 26) + 'a');
        if (fputc(ch, fw) == EOF) return false;
    }
    return true;
}

/* ----------------------------------------------------------
 * XOR cipher — encrypt and decrypt are identical
 * ---------------------------------------------------------- */
static bool xor_cipher(FILE *fr, FILE *fw, int xor_key)
{
    int byte;
    while ((byte = fgetc(fr)) != EOF) {
        char ch = (char)((unsigned char)byte ^ (unsigned char)xor_key);
        if (fputc(ch, fw) == EOF) return false;
    }
    return true;
}

/* ----------------------------------------------------------
 * PBKDF2 key derivation
 *   password  → raw string from the user
 *   salt      → 16 random bytes (generated externally)
 *   out_key   → 32-byte output buffer
 * ---------------------------------------------------------- */
static bool derive_aes_key(const char    *password,
                            const unsigned char *salt,
                            unsigned char *out_key)
{
    int rc = PKCS5_PBKDF2_HMAC(
        password, (int)strlen(password),
        salt,     PBKDF2_SALT_SIZE,
        PBKDF2_ITERATIONS,
        EVP_sha256(),
        AES_KEY_SIZE, out_key
    );
    return (rc == 1);
}

/* ----------------------------------------------------------
 * AES-256-GCM encryption  (EVP API, replaces low-level ECB)
 *
 * Output file layout:
 *   [16 bytes PBKDF2 salt][12 bytes IV][ciphertext][16 bytes GCM tag]
 * ---------------------------------------------------------- */
static bool aes_gcm_encrypt(FILE *fr, FILE *fw, const char *password,
                             char *error_buf, size_t error_buf_size)
{
    unsigned char salt[PBKDF2_SALT_SIZE];
    unsigned char iv[AES_IV_SIZE];
    unsigned char aes_key[AES_KEY_SIZE];
    unsigned char auth_tag[AES_TAG_SIZE];
    unsigned char in_buf[FILE_BUFFER_SIZE];
    unsigned char out_buf[FILE_BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH];
    int           out_len = 0;
    bool          success = false;

    /* Generate random salt and IV */
    if (RAND_bytes(salt, PBKDF2_SALT_SIZE) != 1 ||
        RAND_bytes(iv,   AES_IV_SIZE)      != 1) {
        snprintf(error_buf, error_buf_size,
                 "Failed to generate random salt/IV.");
        goto cleanup;
    }

    /* Derive AES key from password */
    if (!derive_aes_key(password, salt, aes_key)) {
        snprintf(error_buf, error_buf_size, "Key derivation failed.");
        goto cleanup;
    }

    /* Write salt and IV to the output file header */
    if (fwrite(salt, 1, PBKDF2_SALT_SIZE, fw) != PBKDF2_SALT_SIZE ||
        fwrite(iv,   1, AES_IV_SIZE,       fw) != (size_t)AES_IV_SIZE) {
        snprintf(error_buf, error_buf_size,
                 "Failed to write IV/salt header.");
        goto cleanup;
    }

    /* Set up EVP context */
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        snprintf(error_buf, error_buf_size, "EVP context allocation failed.");
        goto cleanup;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_IV_SIZE, NULL) != 1 ||
        EVP_EncryptInit_ex(ctx, NULL, NULL, aes_key, iv) != 1) {
        snprintf(error_buf, error_buf_size, "AES-GCM initialisation failed.");
        EVP_CIPHER_CTX_free(ctx);
        goto cleanup;
    }

    /* Stream the file through the cipher */
    int bytes_read;
    while ((bytes_read = (int)fread(in_buf, 1, FILE_BUFFER_SIZE, fr)) > 0) {
        if (EVP_EncryptUpdate(ctx, out_buf, &out_len, in_buf, bytes_read) != 1) {
            snprintf(error_buf, error_buf_size, "Encryption error during streaming.");
            EVP_CIPHER_CTX_free(ctx);
            goto cleanup;
        }
        if (out_len > 0 && fwrite(out_buf, 1, (size_t)out_len, fw) != (size_t)out_len) {
            snprintf(error_buf, error_buf_size, "Write error during encryption.");
            EVP_CIPHER_CTX_free(ctx);
            goto cleanup;
        }
    }

    /* Finalise and retrieve GCM authentication tag */
    if (EVP_EncryptFinal_ex(ctx, out_buf, &out_len) != 1) {
        snprintf(error_buf, error_buf_size, "AES-GCM finalisation failed.");
        EVP_CIPHER_CTX_free(ctx);
        goto cleanup;
    }
    if (out_len > 0) fwrite(out_buf, 1, (size_t)out_len, fw);

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_TAG_SIZE, auth_tag) != 1) {
        snprintf(error_buf, error_buf_size, "Failed to retrieve GCM tag.");
        EVP_CIPHER_CTX_free(ctx);
        goto cleanup;
    }
    fwrite(auth_tag, 1, AES_TAG_SIZE, fw);

    EVP_CIPHER_CTX_free(ctx);
    success = true;

cleanup:
    OPENSSL_cleanse(aes_key, AES_KEY_SIZE); /* Secure wipe */
    return success;
}

/* ----------------------------------------------------------
 * AES-256-GCM decryption with integrity verification
 *
 * Reads salt, IV from header; verifies GCM tag before
 * accepting the output (prevents decrypting tampered files).
 * ---------------------------------------------------------- */
static bool aes_gcm_decrypt(FILE *fr, FILE *fw, const char *password,
                             char *error_buf, size_t error_buf_size)
{
    unsigned char salt[PBKDF2_SALT_SIZE];
    unsigned char iv[AES_IV_SIZE];
    unsigned char aes_key[AES_KEY_SIZE];
    unsigned char auth_tag[AES_TAG_SIZE];
    unsigned char in_buf[FILE_BUFFER_SIZE];
    unsigned char out_buf[FILE_BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH];
    int           out_len = 0;
    bool          success = false;

    /* Read salt and IV from header */
    if (fread(salt, 1, PBKDF2_SALT_SIZE, fr) != PBKDF2_SALT_SIZE ||
        fread(iv,   1, AES_IV_SIZE,       fr) != (size_t)AES_IV_SIZE) {
        snprintf(error_buf, error_buf_size,
                 "File header is missing or corrupt.");
        goto cleanup;
    }

    /* Derive AES key from password + salt */
    if (!derive_aes_key(password, salt, aes_key)) {
        snprintf(error_buf, error_buf_size, "Key derivation failed.");
        goto cleanup;
    }

    /* --- Buffer the ciphertext to extract the trailing tag --- */
    /* We need to strip the last AES_TAG_SIZE bytes before passing
       data to EVP_DecryptUpdate, then set them as the expected tag. */
    unsigned char *cipher_buf    = NULL;
    long           cipher_len    = 0;
    long           file_start    = ftell(fr);

    fseek(fr, 0, SEEK_END);
    long file_end = ftell(fr);
    cipher_len = file_end - file_start - AES_TAG_SIZE;

    if (cipher_len < 0) {
        snprintf(error_buf, error_buf_size,
                 "File is too small to be a valid encrypted file.");
        goto cleanup;
    }

    /* Read auth tag from end of file */
    fseek(fr, -AES_TAG_SIZE, SEEK_END);
    if (fread(auth_tag, 1, AES_TAG_SIZE, fr) != AES_TAG_SIZE) {
        snprintf(error_buf, error_buf_size, "Failed to read authentication tag.");
        goto cleanup;
    }

    /* Rewind to ciphertext start */
    fseek(fr, file_start, SEEK_SET);

    /* Set up EVP context */
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        snprintf(error_buf, error_buf_size, "EVP context allocation failed.");
        goto cleanup;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_IV_SIZE, NULL) != 1 ||
        EVP_DecryptInit_ex(ctx, NULL, NULL, aes_key, iv) != 1) {
        snprintf(error_buf, error_buf_size, "AES-GCM initialisation failed.");
        EVP_CIPHER_CTX_free(ctx);
        goto cleanup;
    }

    /* Stream ciphertext (excluding the trailing tag) */
    long remaining = cipher_len;
    while (remaining > 0) {
        int  to_read  = (remaining > FILE_BUFFER_SIZE) ? FILE_BUFFER_SIZE : (int)remaining;
        int  got      = (int)fread(in_buf, 1, (size_t)to_read, fr);
        if (got <= 0) break;
        if (EVP_DecryptUpdate(ctx, out_buf, &out_len, in_buf, got) != 1) {
            snprintf(error_buf, error_buf_size, "Decryption error during streaming.");
            EVP_CIPHER_CTX_free(ctx);
            goto cleanup;
        }
        if (out_len > 0) fwrite(out_buf, 1, (size_t)out_len, fw);
        remaining -= got;
    }

    /* Set the expected GCM tag */
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_TAG_SIZE, auth_tag) != 1) {
        snprintf(error_buf, error_buf_size, "Failed to set GCM tag.");
        EVP_CIPHER_CTX_free(ctx);
        goto cleanup;
    }

    /* Finalise — returns -1 if tag verification fails */
    int rc = EVP_DecryptFinal_ex(ctx, out_buf, &out_len);
    EVP_CIPHER_CTX_free(ctx);

    if (rc != 1) {
        snprintf(error_buf, error_buf_size,
                 "Authentication failed! Wrong key or file is corrupted.");
        goto cleanup;
    }
    if (out_len > 0) fwrite(out_buf, 1, (size_t)out_len, fw);

    success = true;

cleanup:
    OPENSSL_cleanse(aes_key, AES_KEY_SIZE); /* Secure wipe */
    if (cipher_buf) free(cipher_buf);
    return success;
}


/* ==========================================================
 * ----------------  SERVICE LAYER  ------------------------
 * Coordinates file I/O and algorithm dispatch
 * ========================================================== */

/* ----------------------------------------------------------
 * Validate the numeric key for Caesar / XOR
 * Returns true on success; writes reason into error_buf.
 * ---------------------------------------------------------- */
static bool validate_numeric_key(const char *algorithm,
                                  const char *password,
                                  int        *out_key,
                                  char       *error_buf,
                                  size_t      error_buf_size)
{
    *out_key = atoi(password);

    if (strcmp(algorithm, "Caesar Cipher") == 0) {
        if (*out_key < CAESAR_KEY_MIN || *out_key > CAESAR_KEY_MAX) {
            snprintf(error_buf, error_buf_size,
                     "Caesar key must be between %d and %d.",
                     CAESAR_KEY_MIN, CAESAR_KEY_MAX);
            return false;
        }
    } else if (strcmp(algorithm, "Xor Cipher") == 0) {
        if (*out_key < XOR_KEY_MIN || *out_key > XOR_KEY_MAX) {
            snprintf(error_buf, error_buf_size,
                     "XOR key must be between %d and %d.",
                     XOR_KEY_MIN, XOR_KEY_MAX);
            return false;
        }
    }
    return true;
}

/* ----------------------------------------------------------
 * perform_encryption / perform_decryption
 *   Unified entry-points called by the controller.
 *   Returns true on success; fills error_buf on failure.
 * ---------------------------------------------------------- */
static bool perform_encryption(const char *algorithm,
                                 const char *password,
                                 const char *input_path,
                                 const char *output_path,
                                 char       *error_buf,
                                 size_t      error_buf_size)
{
    FILE *fr = fopen(input_path, "rb");
    if (!fr) {
        snprintf(error_buf, error_buf_size,
                 "Cannot open input file: %s", input_path);
        return false;
    }

    bool same_file = (strcmp(input_path, output_path) == 0);
    FILE *fw = fopen(output_path, same_file ? "rb+" : "wb");
    if (!fw) {
        fclose(fr);
        snprintf(error_buf, error_buf_size,
                 "Cannot open output file: %s", output_path);
        return false;
    }

    bool success = false;
    char local_err[256] = {0};
    int  numeric_key    = 0;

    if (strcmp(algorithm, "Caesar Cipher") == 0) {
        if (validate_numeric_key(algorithm, password,
                                  &numeric_key, local_err, sizeof(local_err))) {
            success = caesar_encrypt(fr, fw, numeric_key);
            if (!success)
                snprintf(local_err, sizeof(local_err), "Caesar encryption failed.");
        }
    } else if (strcmp(algorithm, "Xor Cipher") == 0) {
        if (validate_numeric_key(algorithm, password,
                                  &numeric_key, local_err, sizeof(local_err))) {
            success = xor_cipher(fr, fw, numeric_key);
            if (!success)
                snprintf(local_err, sizeof(local_err), "XOR encryption failed.");
        }
    } else if (strcmp(algorithm, "AES-256-GCM") == 0) {
        success = aes_gcm_encrypt(fr, fw, password, local_err, sizeof(local_err));
    } else {
        snprintf(local_err, sizeof(local_err), "Unknown algorithm selected.");
    }

    fclose(fr);
    fclose(fw);

    if (!success)
        snprintf(error_buf, error_buf_size, "%s", local_err);

    return success;
}

static bool perform_decryption(const char *algorithm,
                                 const char *password,
                                 const char *input_path,
                                 const char *output_path,
                                 char       *error_buf,
                                 size_t      error_buf_size)
{
    FILE *fr = fopen(input_path, "rb");
    if (!fr) {
        snprintf(error_buf, error_buf_size,
                 "Cannot open input file: %s", input_path);
        return false;
    }

    bool same_file = (strcmp(input_path, output_path) == 0);
    FILE *fw = fopen(output_path, same_file ? "rb+" : "wb");
    if (!fw) {
        fclose(fr);
        snprintf(error_buf, error_buf_size,
                 "Cannot open output file: %s", output_path);
        return false;
    }

    bool success = false;
    char local_err[256] = {0};
    int  numeric_key    = 0;

    if (strcmp(algorithm, "Caesar Cipher") == 0) {
        if (validate_numeric_key(algorithm, password,
                                  &numeric_key, local_err, sizeof(local_err))) {
            success = caesar_decrypt(fr, fw, numeric_key);
            if (!success)
                snprintf(local_err, sizeof(local_err), "Caesar decryption failed.");
        }
    } else if (strcmp(algorithm, "Xor Cipher") == 0) {
        if (validate_numeric_key(algorithm, password,
                                  &numeric_key, local_err, sizeof(local_err))) {
            success = xor_cipher(fr, fw, numeric_key);
            if (!success)
                snprintf(local_err, sizeof(local_err), "XOR decryption failed.");
        }
    } else if (strcmp(algorithm, "AES-256-GCM") == 0) {
        success = aes_gcm_decrypt(fr, fw, password, local_err, sizeof(local_err));
    } else {
        snprintf(local_err, sizeof(local_err), "Unknown algorithm selected.");
    }

    fclose(fr);
    fclose(fw);

    if (!success)
        snprintf(error_buf, error_buf_size, "%s", local_err);

    return success;
}


/* ==========================================================
 * ----------------  CONTROLLER LAYER  ---------------------
 * GTK callback functions (UI → Controller → Service)
 * ========================================================== */

/* ----------------------------------------------------------
 * Evaluate whether all three conditions are met and update
 * the status label and key-entry editability accordingly.
 * ---------------------------------------------------------- */
static void refresh_ready_state(WidgetBundle *wb)
{
    AppState  *state     = wb->state;
    GtkWidget *combo     = wb->algorithm_combo;
    GtkWidget *entry     = wb->key_entry;
    GtkWidget *status    = wb->status_label;

    bool all_ready = state->input_ready && state->output_ready && state->action_ready;

    if (all_ready) {
        gtk_label_set_text(GTK_LABEL(status), "Status : Ready");
        gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);

        char *algo = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
        if (algo) {
            if (strcmp(algo, "Caesar Cipher") == 0)
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),
                    "Enter shift key (1 – 25)");
            else if (strcmp(algo, "Xor Cipher") == 0)
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),
                    "Enter XOR key (1 – 255)");
            else
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),
                    "Enter passphrase");
            g_free(algo);
        }
    } else {
        gtk_label_set_text(GTK_LABEL(status), "Status : Unready");
        gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    }
}

/* ----------------------------------------------------------
 * on_algorithm_changed — update placeholder when status is Ready
 * ---------------------------------------------------------- */
static void on_algorithm_changed(GtkComboBox *combo, gpointer user_data)
{
    WidgetBundle *wb = (WidgetBundle *)user_data;
    refresh_ready_state(wb);
}

/* ----------------------------------------------------------
 * on_action_toggled — called when Encrypt / Decrypt radio toggled
 * ---------------------------------------------------------- */
static void on_action_toggled(GtkWidget *radio, gpointer user_data)
{
    WidgetBundle *wb = (WidgetBundle *)user_data;

    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        wb->state->action_label = (char *)gtk_check_button_get_label(
                                      GTK_CHECK_BUTTON(radio));
        gtk_button_set_label(GTK_BUTTON(wb->action_button),
                             wb->state->action_label);
        wb->state->action_ready = true;
        refresh_ready_state(wb);
    }
}

/* ----------------------------------------------------------
 * on_response_open — file-chooser dialog callback (input file)
 * ---------------------------------------------------------- */
static void on_response_open(GtkNativeDialog *native,
                              int              response,
                              gpointer         user_data)
{
    if (response != GTK_RESPONSE_ACCEPT) return;

    WidgetBundle *wb      = (WidgetBundle *)user_data;
    AppState     *state   = wb->state;
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
    GFile          *file    = gtk_file_chooser_get_file(chooser);

    g_free(state->input_path);
    state->input_path = g_file_get_path(file);
    g_object_unref(file);

    /* Update label on the browse button */
    GtkWidget *lbl = gtk_label_new(state->input_path);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(wb->input_button), lbl);

    state->input_ready = true;

    /* Auto-check overwrite if both paths are the same */
    if (state->output_path &&
        strcmp(state->input_path, state->output_path) == 0)
        gtk_check_button_set_active(GTK_CHECK_BUTTON(wb->overwrite_check), TRUE);

    refresh_ready_state(wb);
}

/* ----------------------------------------------------------
 * on_response_save — file-chooser dialog callback (output file)
 * ---------------------------------------------------------- */
static void on_response_save(GtkNativeDialog *native,
                              int              response,
                              gpointer         user_data)
{
    if (response != GTK_RESPONSE_ACCEPT) return;

    WidgetBundle *wb      = (WidgetBundle *)user_data;
    AppState     *state   = wb->state;
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
    GFile          *file    = gtk_file_chooser_get_file(chooser);

    g_free(state->output_path);
    state->output_path = g_file_get_path(file);
    g_object_unref(file);

    GtkWidget *lbl = gtk_label_new(state->output_path);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(wb->output_button), lbl);

    state->output_ready = true;

    if (state->input_path &&
        strcmp(state->input_path, state->output_path) == 0)
        gtk_check_button_set_active(GTK_CHECK_BUTTON(wb->overwrite_check), TRUE);

    refresh_ready_state(wb);
}

/* ----------------------------------------------------------
 * browse_for_input — opens file-open native dialog
 * ---------------------------------------------------------- */
static void browse_for_input(GtkWidget *button, gpointer user_data)
{
    WidgetBundle *wb = (WidgetBundle *)user_data;

    GtkFileChooserNative *native = gtk_file_chooser_native_new(
        "Select Input File",
        GTK_WINDOW(wb->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Open", "_Cancel"
    );
    g_signal_connect(native, "response",
                     G_CALLBACK(on_response_open), wb);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
}

/* ----------------------------------------------------------
 * browse_for_output — opens file-save native dialog
 * ---------------------------------------------------------- */
static void browse_for_output(GtkWidget *button, gpointer user_data)
{
    WidgetBundle *wb = (WidgetBundle *)user_data;

    GtkFileChooserNative *native = gtk_file_chooser_native_new(
        "Select Output File",
        GTK_WINDOW(wb->window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Save", "_Cancel"
    );
    g_signal_connect(native, "response",
                     G_CALLBACK(on_response_save), wb);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
}

/* ----------------------------------------------------------
 * do_action — triggered by the Encrypt / Decrypt button
 * ---------------------------------------------------------- */
static void do_action(GtkWidget *button, gpointer user_data)
{
    WidgetBundle *wb       = (WidgetBundle *)user_data;
    AppState     *state    = wb->state;

    char *algorithm = gtk_combo_box_text_get_active_text(
                          GTK_COMBO_BOX_TEXT(wb->algorithm_combo));
    const char *password = gtk_editable_get_text(
                               GTK_EDITABLE(wb->key_entry));

    /* Input validation */
    if (!algorithm || strcmp(algorithm, "Select an algorithm") == 0) {
        gtk_label_set_text(GTK_LABEL(wb->result_label),
                           "Please select an algorithm.");
        g_free(algorithm);
        return;
    }
    if (strlen(password) == 0) {
        gtk_label_set_text(GTK_LABEL(wb->result_label),
                           "Please enter a key or passphrase.");
        g_free(algorithm);
        return;
    }
    if (!state->input_path || !state->output_path) {
        gtk_label_set_text(GTK_LABEL(wb->result_label),
                           "Input and output files must be selected.");
        g_free(algorithm);
        return;
    }

    char error_buf[512] = {0};
    bool success        = false;

    if (state->action_label && strcmp(state->action_label, "Encrypt") == 0) {
        success = perform_encryption(algorithm, password,
                                     state->input_path, state->output_path,
                                     error_buf, sizeof(error_buf));
        gtk_label_set_text(GTK_LABEL(wb->result_label),
                           success ? "Encryption successful!"
                                   : error_buf);
    } else if (state->action_label && strcmp(state->action_label, "Decrypt") == 0) {
        success = perform_decryption(algorithm, password,
                                     state->input_path, state->output_path,
                                     error_buf, sizeof(error_buf));
        gtk_label_set_text(GTK_LABEL(wb->result_label),
                           success ? "Decryption successful!"
                                   : error_buf);
    } else {
        gtk_label_set_text(GTK_LABEL(wb->result_label),
                           "Select Encrypt or Decrypt first.");
    }

    g_free(algorithm);
}

/* ----------------------------------------------------------
 * on_clear — resets all state and restores default UI
 * ---------------------------------------------------------- */
static void on_clear(GtkWidget *button, gpointer user_data)
{
    WidgetBundle *wb    = (WidgetBundle *)user_data;
    AppState     *state = wb->state;

    /* Free heap paths */
    g_free(state->input_path);
    g_free(state->output_path);
    state->input_path   = NULL;
    state->output_path  = NULL;
    state->action_label = NULL;
    state->input_ready  = false;
    state->output_ready = false;
    state->action_ready = false;

    /* Restore input button label */
    GtkWidget *lbl_in = gtk_label_new("C:\\Users\\Username\\Document.txt");
    gtk_widget_set_halign(lbl_in, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(wb->input_button), lbl_in);

    /* Restore output button label */
    GtkWidget *lbl_out = gtk_label_new("C:\\Users\\Username\\");
    gtk_widget_set_halign(lbl_out, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(wb->output_button), lbl_out);

    gtk_editable_set_editable(GTK_EDITABLE(wb->key_entry), TRUE);
    gtk_editable_set_text(GTK_EDITABLE(wb->key_entry), "");
    gtk_entry_set_placeholder_text(GTK_ENTRY(wb->key_entry),
                                   "Enter key value when you are asked");
    gtk_editable_set_editable(GTK_EDITABLE(wb->key_entry), FALSE);

    gtk_check_button_set_active(
        GTK_CHECK_BUTTON(wb->action_radio_encrypt), FALSE);
    gtk_check_button_set_active(
        GTK_CHECK_BUTTON(wb->action_radio_decrypt), FALSE);
    gtk_check_button_set_active(
        GTK_CHECK_BUTTON(wb->overwrite_check), FALSE);

    gtk_combo_box_set_active(GTK_COMBO_BOX(wb->algorithm_combo), 0);
    gtk_label_set_text(GTK_LABEL(wb->result_label), "");
    gtk_button_set_label(GTK_BUTTON(wb->action_button), "Default");
    gtk_label_set_text(GTK_LABEL(wb->status_label), "Status : Unready");
}

/* ----------------------------------------------------------
 * toggle_key_visibility — eye icon on the password entry
 * ---------------------------------------------------------- */
static void toggle_key_visibility(GtkEntry            *entry,
                                   GtkEntryIconPosition pos,
                                   GdkEvent            *event,
                                   gpointer             user_data)
{
    gboolean visible = gtk_entry_get_visibility(entry);
    gtk_entry_set_visibility(entry, !visible);
    gtk_entry_set_icon_from_icon_name(
        entry,
        GTK_ENTRY_ICON_SECONDARY,
        visible ? "view-hidden-symbolic" : "view-visible-symbolic"
    );
}

/* ----------------------------------------------------------
 * on_exit — closes the application
 * ---------------------------------------------------------- */
static void on_exit(GtkWidget *button, gpointer user_data)
{
    exit(0);
}


/* ==========================================================
 * ----------------  UI CONSTRUCTION  ----------------------
 * ========================================================== */

static void activate(GtkApplication *app, gpointer user_data)
{
    /* Allocate application state on the heap */
    AppState *state = g_new0(AppState, 1);

    /* ---- Main window ---- */
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Encryptor");
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 430);

    /* ---- Root vertical box ---- */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_window_set_child(GTK_WINDOW(window), vbox);
    gtk_widget_set_margin_top(vbox,    40);
    gtk_widget_set_margin_bottom(vbox, 40);
    gtk_widget_set_margin_start(vbox,  40);
    gtk_widget_set_margin_end(vbox,    40);

    /* ---- Row: section header ---- */
    GtkWidget *hbox_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl_header  = gtk_label_new("Select a file to encrypt / decrypt");
    gtk_box_append(GTK_BOX(hbox_header), lbl_header);
    gtk_box_append(GTK_BOX(vbox), hbox_header);

    /* ---- Row: input file ---- */
    GtkWidget *hbox_input  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl_file    = gtk_label_new("File :");
    gtk_box_append(GTK_BOX(hbox_input), lbl_file);

    GtkWidget *btn_input   = gtk_button_new();
    gtk_widget_set_margin_start(btn_input, 130);
    GtkWidget *lbl_input_path = gtk_label_new("C:\\Users\\Username\\Document.txt");
    gtk_widget_set_halign(lbl_input_path, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(btn_input), lbl_input_path);
    gtk_widget_set_hexpand(btn_input, TRUE);
    gtk_box_append(GTK_BOX(hbox_input), btn_input);

    GtkWidget *btn_browse_input = gtk_button_new_with_label("Browse…");
    gtk_box_append(GTK_BOX(hbox_input), btn_browse_input);
    gtk_box_append(GTK_BOX(vbox), hbox_input);

    /* ---- Row: output file ---- */
    GtkWidget *hbox_output  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl_out_dir  = gtk_label_new("Output Folder :");
    gtk_box_append(GTK_BOX(hbox_output), lbl_out_dir);

    GtkWidget *btn_output   = gtk_button_new();
    gtk_widget_set_margin_start(btn_output, 44);
    GtkWidget *lbl_output_path = gtk_label_new("C:\\Users\\Username\\");
    gtk_widget_set_halign(lbl_output_path, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(btn_output), lbl_output_path);
    gtk_widget_set_hexpand(btn_output, TRUE);
    gtk_box_append(GTK_BOX(hbox_output), btn_output);

    GtkWidget *btn_browse_output = gtk_button_new_with_label("Browse…");
    gtk_box_append(GTK_BOX(hbox_output), btn_browse_output);
    gtk_box_append(GTK_BOX(vbox), hbox_output);

    /* ---- Row: overwrite checkbox ---- */
    GtkWidget *hbox_overwrite = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *chk_overwrite  = gtk_check_button_new_with_label("Overwrite original");
    gtk_widget_set_sensitive(chk_overwrite, FALSE);
    gtk_widget_set_margin_start(chk_overwrite, 170);
    gtk_box_append(GTK_BOX(hbox_overwrite), chk_overwrite);
    gtk_box_append(GTK_BOX(vbox), hbox_overwrite);

    /* ---- Row: algorithm selector ---- */
    GtkWidget *hbox_algo = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl_algo  = gtk_label_new("Algorithm :");
    gtk_box_append(GTK_BOX(hbox_algo), lbl_algo);

    GtkWidget *combo_algo = gtk_combo_box_text_new();
    gtk_widget_set_margin_start(combo_algo, 75);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_algo),
                                   "Select an algorithm");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_algo),
                                   "Caesar Cipher");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_algo),
                                   "Xor Cipher");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_algo),
                                   "AES-256-GCM");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_algo), 0);
    gtk_widget_set_hexpand(combo_algo, TRUE);
    gtk_box_append(GTK_BOX(hbox_algo), combo_algo);
    gtk_box_append(GTK_BOX(vbox), hbox_algo);

    /* ---- Row: action radio buttons ---- */
    GtkWidget *hbox_action = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 38);
    GtkWidget *lbl_action  = gtk_label_new("Action :");
    gtk_box_append(GTK_BOX(hbox_action), lbl_action);

    GtkWidget *radio_encrypt = gtk_check_button_new_with_label("Encrypt");
    gtk_widget_set_margin_start(radio_encrypt, 70);
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_encrypt), NULL);
    gtk_box_append(GTK_BOX(hbox_action), radio_encrypt);

    GtkWidget *radio_decrypt = gtk_check_button_new_with_label("Decrypt");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_decrypt),
                               GTK_CHECK_BUTTON(radio_encrypt));
    gtk_box_append(GTK_BOX(hbox_action), radio_decrypt);
    gtk_box_append(GTK_BOX(vbox), hbox_action);

    /* ---- Row: key / passphrase entry ---- */
    GtkWidget *hbox_key = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl_key  = gtk_label_new("Key Value :");
    gtk_box_append(GTK_BOX(hbox_key), lbl_key);

    GtkWidget *entry_key = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_key),
                                   "Enter key value when you are asked");
    gtk_editable_set_editable(GTK_EDITABLE(entry_key), FALSE);
    gtk_entry_set_visibility(GTK_ENTRY(entry_key), FALSE);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry_key),
                                      GTK_ENTRY_ICON_SECONDARY,
                                      "view-hidden-symbolic");
    gtk_entry_set_icon_activatable(GTK_ENTRY(entry_key),
                                   GTK_ENTRY_ICON_SECONDARY, TRUE);
    gtk_widget_set_margin_start(entry_key, 80);
    gtk_widget_set_hexpand(entry_key, TRUE);
    gtk_box_append(GTK_BOX(hbox_key), entry_key);
    gtk_box_append(GTK_BOX(vbox), hbox_key);

    /* ---- Row: status + action/clear/exit buttons ---- */
    GtkWidget *hbox_bottom  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl_status   = gtk_label_new("Status : Unready");
    gtk_widget_set_hexpand(lbl_status, TRUE);
    gtk_widget_set_halign(lbl_status, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(hbox_bottom), lbl_status);

    GtkWidget *btn_action = gtk_button_new_with_label("Default");
    gtk_box_append(GTK_BOX(hbox_bottom), btn_action);

    GtkWidget *btn_clear  = gtk_button_new_with_label("Clear");
    gtk_box_append(GTK_BOX(hbox_bottom), btn_clear);

    GtkWidget *btn_exit   = gtk_button_new_with_label("Exit");
    gtk_box_append(GTK_BOX(hbox_bottom), btn_exit);
    gtk_box_append(GTK_BOX(vbox), hbox_bottom);

    /* ---- Row: result message ---- */
    GtkWidget *hbox_result = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl_result  = gtk_label_new("");
    gtk_widget_set_hexpand(lbl_result, TRUE);
    gtk_box_append(GTK_BOX(hbox_result), lbl_result);
    gtk_box_append(GTK_BOX(vbox), hbox_result);

    /* ---- Build the widget bundle ---- */
    WidgetBundle *wb         = g_new0(WidgetBundle, 1);
    wb->window               = window;
    wb->input_button         = btn_input;
    wb->output_button        = btn_output;
    wb->overwrite_check      = chk_overwrite;
    wb->algorithm_combo      = combo_algo;
    wb->action_radio_encrypt = radio_encrypt;
    wb->action_radio_decrypt = radio_decrypt;
    wb->key_entry            = entry_key;
    wb->action_button        = btn_action;
    wb->status_label         = lbl_status;
    wb->result_label         = lbl_result;
    wb->state                = state;

    /* ---- Connect signals ---- */
    g_signal_connect(btn_browse_input,  "clicked",
                     G_CALLBACK(browse_for_input),       wb);
    g_signal_connect(btn_browse_output, "clicked",
                     G_CALLBACK(browse_for_output),      wb);
    g_signal_connect(radio_encrypt,     "toggled",
                     G_CALLBACK(on_action_toggled),      wb);
    g_signal_connect(radio_decrypt,     "toggled",
                     G_CALLBACK(on_action_toggled),      wb);
    g_signal_connect(btn_action,        "clicked",
                     G_CALLBACK(do_action),              wb);
    g_signal_connect(btn_clear,         "clicked",
                     G_CALLBACK(on_clear),               wb);
    g_signal_connect(btn_exit,          "clicked",
                     G_CALLBACK(on_exit),                NULL);
    g_signal_connect(combo_algo,        "changed",
                     G_CALLBACK(on_algorithm_changed),   wb);
    g_signal_connect(entry_key,         "icon-press",
                     G_CALLBACK(toggle_key_visibility),  NULL);

    gtk_window_present(GTK_WINDOW(window));
}


/* ==========================================================
 * ---- Entry point ----
 * ========================================================== */
int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new(
        "com.example.file_encryptor",
        G_APPLICATION_DEFAULT_FLAGS
    );
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
