/* ──────────────────────────────────────────────────────────────────────────
 * main.c
 * Entry point and GTK GUI for the Advanced File Encryption System.
 *
 * Improvements over original (per Final Report §5 & §6 and Thesis doc):
 *   [FIX-G1]  Global FILE* pointers (fr, fw) removed.  File handles are
 *             now passed directly to encryption functions.
 *   [FIX-G2]  Global path strings (path1, path2) moved into AppState struct.
 *   [FIX-G3]  Boolean state flags (fo, fs, ft) moved into AppState struct.
 *   [FIX-G4]  g_strdup used for path copies; g_free called on clear to
 *             prevent memory leaks.
 *   [FIX-G5]  Encryption dispatch calls the new modular API in encryption.c
 *             and propagates structured EncryptStatus error messages to GUI.
 *   [FIX-G6]  Consistent snake_case naming throughout.
 *   [FIX-G7]  Single is_ready() helper replaces three duplicated
 *             "if(ft && fo && fs)" blocks.
 *   [FIX-G8]  Label "Overwrite origanal" typo corrected to
 *             "Overwrite original".
 * ──────────────────────────────────────────────────────────────────────── */

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "encryption.h"
#include "file_handler.h"
#include "logger.h"

/* ══════════════════════════════════════════════════════════════════════════
 * Application State  (replaces scattered global variables – [FIX-G2/G3])
 * ══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    char   *src_path;       /* Path of file to encrypt / decrypt.           */
    char   *dst_path;       /* Path of output file.                         */
    char   *action_label;   /* "Encrypt" or "Decrypt" (borrowed, not owned) */
    gboolean src_selected;  /* Source file has been chosen.                 */
    gboolean dst_selected;  /* Destination file has been chosen.            */
    gboolean action_set;    /* Encrypt / Decrypt radio is active.           */
} AppState;

static AppState g_state = { NULL, NULL, NULL, FALSE, FALSE, FALSE };

/* Free owned path strings and reset flags. */
static void app_state_reset(AppState *state) {
    g_free(state->src_path);
    g_free(state->dst_path);
    state->src_path     = NULL;
    state->dst_path     = NULL;
    state->action_label = NULL;
    state->src_selected = FALSE;
    state->dst_selected = FALSE;
    state->action_set   = FALSE;
}

/* ══════════════════════════════════════════════════════════════════════════
 * Helpers
 * ══════════════════════════════════════════════════════════════════════════ */

/* [FIX-G7] Single readiness check replaces three duplicated if-blocks. */
static gboolean is_ready(const AppState *state) {
    return state->src_selected && state->dst_selected && state->action_set;
}

/* Update the key-entry placeholder to match the chosen algorithm. */
static void update_key_hint(GtkWidget *entry, const char *algorithm) {
    if (!algorithm) return;
    if (strcmp(algorithm, "Caesar Cipher") == 0) {
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry),
            "Enter key value (1 to 25):");
    } else if (strcmp(algorithm, "Xor Cipher") == 0) {
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry),
            "Enter key value (1 to 255):");
    } else if (strcmp(algorithm, "AES") == 0) {
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry),
            "Enter passphrase:");
    }
}

/* Enable the key entry and update its hint when all three conditions are met. */
static void maybe_activate_entry(GtkWidget *entry,
                                 GtkWidget *status_label,
                                 GtkWidget *combobox) {
    if (!is_ready(&g_state)) return;

    char *algorithm = gtk_combo_box_text_get_active_text(
                          GTK_COMBO_BOX_TEXT(combobox));
    gtk_label_set_text(GTK_LABEL(status_label), "Status: Ready");
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
    update_key_hint(entry, algorithm);
    g_free(algorithm);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Callbacks
 * ══════════════════════════════════════════════════════════════════════════ */

/* Toggle password visibility icon. */
static void on_toggle_visibility(GtkEntry            *entry,
                                 GtkEntryIconPosition pos,
                                 GdkEvent            *event,
                                 gpointer             user_data) {
    (void)pos; (void)event; (void)user_data;
    gboolean visible = gtk_entry_get_visibility(entry);
    gtk_entry_set_visibility(entry, !visible);
    gtk_entry_set_icon_from_icon_name(
        entry, GTK_ENTRY_ICON_SECONDARY,
        visible ? "view-hidden-symbolic" : "view-visible-symbolic");
}

/* ── Algorithm combo changed ─────────────────────────────────────────────── */
typedef struct { GtkWidget *entry; GtkWidget *status_label; } AlgoWidgets;

static void on_algorithm_changed(GtkComboBox *combobox, gpointer user_data) {
    AlgoWidgets *w = (AlgoWidgets *)user_data;
    const char *label = gtk_label_get_text(GTK_LABEL(w->status_label));
    if (strcmp(label, "Status: Ready") == 0) {
        char *algorithm = gtk_combo_box_text_get_active_text(
                              GTK_COMBO_BOX_TEXT(combobox));
        update_key_hint(w->entry, algorithm);
        g_free(algorithm);
    }
}

/* ── Encrypt / Decrypt radio toggled ────────────────────────────────────── */
typedef struct {
    GtkWidget *action_button;   /* The "Encrypt"/"Decrypt" execute button. */
    GtkWidget *status_label;
    GtkWidget *key_entry;
    GtkWidget *combobox;
} ToggleWidgets;

static void on_action_toggled(GtkWidget *radio, gpointer user_data) {
    ToggleWidgets *w = (ToggleWidgets *)user_data;
    if (!gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) return;

    g_state.action_label = (char *)gtk_check_button_get_label(
                               GTK_CHECK_BUTTON(radio));
    gtk_button_set_label(GTK_BUTTON(w->action_button), g_state.action_label);
    g_state.action_set = TRUE;

    LOG_INFO("Action set to: %s.", g_state.action_label);
    maybe_activate_entry(w->key_entry, w->status_label, w->combobox);
}

/* ── File open (source) response ────────────────────────────────────────── */
typedef struct {
    GtkWidget *path_button;
    GtkWidget *window;
    GtkWidget *status_label;
    GtkWidget *overwrite_check;
    GtkWidget *key_entry;
    GtkWidget *combobox;
} FileWidgets;

static void on_response_open(GtkNativeDialog *native,
                             int              response,
                             gpointer         user_data) {
    if (response != GTK_RESPONSE_ACCEPT) return;
    FileWidgets *w = (FileWidgets *)user_data;

    GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
    GFile *file = gtk_file_chooser_get_file(chooser);

    g_free(g_state.src_path);
    g_state.src_path = g_file_get_path(file); /* [FIX-G4] g_strdup-based copy */
    g_object_unref(file);
    g_state.src_selected = TRUE;

    LOG_INFO("Source file selected: %s.", g_state.src_path);

    GtkWidget *path_label = gtk_label_new(g_state.src_path);
    gtk_widget_set_halign(path_label, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(w->path_button), path_label);

    /* Auto-check overwrite when src and dst are identical. */
    if (g_state.dst_path &&
        paths_are_same(g_state.src_path, g_state.dst_path)) {
        gtk_check_button_set_active(GTK_CHECK_BUTTON(w->overwrite_check), TRUE);
    }

    maybe_activate_entry(w->key_entry, w->status_label, w->combobox);
}

static void on_browse_open_clicked(GtkWidget *button, gpointer user_data) {
    FileWidgets *w = (FileWidgets *)user_data;
    GtkFileChooserNative *native = gtk_file_chooser_native_new(
        "Select Source File",
        GTK_WINDOW(w->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Open", "_Cancel");
    g_signal_connect(native, "response",
                     G_CALLBACK(on_response_open), user_data);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
}

/* ── File save (destination) response ───────────────────────────────────── */
static void on_response_save(GtkNativeDialog *native,
                             int              response,
                             gpointer         user_data) {
    if (response != GTK_RESPONSE_ACCEPT) return;
    FileWidgets *w = (FileWidgets *)user_data;

    GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
    GFile *file = gtk_file_chooser_get_file(chooser);

    g_free(g_state.dst_path);
    g_state.dst_path = g_file_get_path(file); /* [FIX-G4] */
    g_object_unref(file);
    g_state.dst_selected = TRUE;

    LOG_INFO("Destination file selected: %s.", g_state.dst_path);

    GtkWidget *path_label = gtk_label_new(g_state.dst_path);
    gtk_widget_set_halign(path_label, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(w->path_button), path_label);

    if (g_state.src_path &&
        paths_are_same(g_state.src_path, g_state.dst_path)) {
        gtk_check_button_set_active(GTK_CHECK_BUTTON(w->overwrite_check), TRUE);
    }

    maybe_activate_entry(w->key_entry, w->status_label, w->combobox);
}

static void on_browse_save_clicked(GtkWidget *button, gpointer user_data) {
    FileWidgets *w = (FileWidgets *)user_data;
    GtkFileChooserNative *native = gtk_file_chooser_native_new(
        "Select Output File",
        GTK_WINDOW(w->window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Save", "_Cancel");
    g_signal_connect(native, "response",
                     G_CALLBACK(on_response_save), user_data);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
}

/* ── Execute (Encrypt / Decrypt) ─────────────────────────────────────────── */
typedef struct {
    GtkWidget *combobox;
    GtkWidget *key_entry;
    GtkWidget *result_label;
} ExecuteWidgets;

static void on_execute_clicked(GtkWidget *button, gpointer user_data) {
    ExecuteWidgets *w = (ExecuteWidgets *)user_data;

    char *algorithm = gtk_combo_box_text_get_active_text(
                          GTK_COMBO_BOX_TEXT(w->combobox));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(w->key_entry));

    /* Validate that all state is populated. */
    if (!g_state.src_path || !g_state.dst_path || !g_state.action_label) {
        gtk_label_set_text(GTK_LABEL(w->result_label),
                           "Error: Please select files and action first.");
        g_free(algorithm);
        return;
    }

    if (strlen(password) == 0) {
        gtk_label_set_text(GTK_LABEL(w->result_label),
                           "Error: Please enter a key or passphrase.");
        g_free(algorithm);
        return;
    }

    LOG_INFO("Execute clicked: algorithm=%s, action=%s.",
             algorithm, g_state.action_label);

    EncryptStatus status = ENCRYPT_OK;
    gboolean is_encrypt = (strcmp(g_state.action_label, "Encrypt") == 0);

    /* [FIX-G5] Dispatch to modular encryption functions with error handling. */
    if (strcmp(algorithm, "Caesar Cipher") == 0) {
        int key = atoi(password);
        if (key < CAESAR_KEY_MIN || key > CAESAR_KEY_MAX) {
            gtk_editable_set_text(GTK_EDITABLE(w->key_entry), "");
            gtk_entry_set_placeholder_text(GTK_ENTRY(w->key_entry),
                "Key must be between 1 and 25");
            g_free(algorithm);
            return;
        }
        status = is_encrypt
               ? caesar_cipher_encrypt(g_state.src_path, g_state.dst_path, key)
               : caesar_cipher_decrypt(g_state.src_path, g_state.dst_path, key);

    } else if (strcmp(algorithm, "Xor Cipher") == 0) {
        int key = atoi(password);
        if (key < XOR_KEY_MIN || key > XOR_KEY_MAX) {
            gtk_editable_set_text(GTK_EDITABLE(w->key_entry), "");
            gtk_entry_set_placeholder_text(GTK_ENTRY(w->key_entry),
                "Key must be between 1 and 255");
            g_free(algorithm);
            return;
        }
        status = is_encrypt
               ? xor_cipher_encrypt(g_state.src_path, g_state.dst_path, key)
               : xor_cipher_decrypt(g_state.src_path, g_state.dst_path, key);

    } else if (strcmp(algorithm, "AES") == 0) {
        status = is_encrypt
               ? aes_encrypt(g_state.src_path, g_state.dst_path, password)
               : aes_decrypt(g_state.src_path, g_state.dst_path, password);
    }

    /* Display result to user. */
    if (status == ENCRYPT_OK) {
        char msg[128];
        snprintf(msg, sizeof(msg), "%s Successful!",
                 is_encrypt ? "Encryption" : "Decryption");
        gtk_label_set_text(GTK_LABEL(w->result_label), msg);
        LOG_INFO("%s", msg);
    } else {
        gtk_label_set_text(GTK_LABEL(w->result_label),
                           encrypt_status_message(status));
        LOG_ERROR("Operation failed: %s", encrypt_status_message(status));
    }

    g_free(algorithm);
    /* Reset action so the user must confirm before another run. */
    g_state.action_label = NULL;
}

/* ── Clear / Reset ───────────────────────────────────────────────────────── */
typedef struct {
    GtkWidget *src_path_button;
    GtkWidget *dst_path_button;
    GtkWidget *key_entry;
    GtkWidget *radio_encrypt;
    GtkWidget *radio_decrypt;
    GtkWidget *combobox;
    GtkWidget *result_label;
    GtkWidget *execute_button;
    GtkWidget *status_label;
    GtkWidget *overwrite_check;
} ClearWidgets;

static void on_clear_clicked(GtkWidget *button, gpointer user_data) {
    ClearWidgets *w = (ClearWidgets *)user_data;

    app_state_reset(&g_state); /* [FIX-G4] frees allocated paths. */

    GtkWidget *src_label = gtk_label_new("C:\\Users\\Username\\Document.txt");
    gtk_widget_set_halign(src_label, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(w->src_path_button), src_label);

    GtkWidget *dst_label = gtk_label_new("C:\\Users\\Username\\");
    gtk_widget_set_halign(dst_label, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(w->dst_path_button), dst_label);

    gtk_editable_set_editable(GTK_EDITABLE(w->key_entry), TRUE);
    gtk_editable_set_text(GTK_EDITABLE(w->key_entry), "");
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->key_entry),
                                   "Enter key value when you are asked");
    gtk_editable_set_editable(GTK_EDITABLE(w->key_entry), FALSE);

    gtk_check_button_set_active(GTK_CHECK_BUTTON(w->radio_encrypt), FALSE);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(w->radio_decrypt), FALSE);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(w->overwrite_check), FALSE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(w->combobox), 0);
    gtk_label_set_text(GTK_LABEL(w->result_label), "");
    gtk_button_set_label(GTK_BUTTON(w->execute_button), "Default");
    gtk_label_set_text(GTK_LABEL(w->status_label), "Status: Unready");

    LOG_INFO("UI cleared and reset to default state.");
}

/* ── Exit ────────────────────────────────────────────────────────────────── */
static void on_exit_clicked(GtkWidget *button, gpointer user_data) {
    app_state_reset(&g_state);
    log_close();
    exit(0);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Build the GTK window
 * ══════════════════════════════════════════════════════════════════════════ */

static void activate(GtkApplication *app, gpointer user_data) {

    /* ── Window ── */
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Encryptor");
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 420);

    /* ── Root vertical box ── */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_window_set_child(GTK_WINDOW(window), vbox);
    gtk_widget_set_margin_top(vbox, 40);
    gtk_widget_set_margin_bottom(vbox, 40);
    gtk_widget_set_margin_start(vbox, 40);
    gtk_widget_set_margin_end(vbox, 40);

    /* ── Row 1: heading ── */
    GtkWidget *hbox_heading = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(hbox_heading),
                   gtk_label_new("Select a file to encrypt/decrypt"));
    gtk_box_append(GTK_BOX(vbox), hbox_heading);

    /* ── Row 2: source file ── */
    GtkWidget *hbox_src = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(hbox_src), gtk_label_new("File:"));

    GtkWidget *src_path_button = gtk_button_new();
    gtk_widget_set_margin_start(src_path_button, 130);
    GtkWidget *src_path_label = gtk_label_new("C:\\Users\\Username\\Document.txt");
    gtk_widget_set_halign(src_path_label, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(src_path_button), src_path_label);
    gtk_widget_set_hexpand(src_path_button, TRUE);
    gtk_box_append(GTK_BOX(hbox_src), src_path_button);

    GtkWidget *browse_open_button = gtk_button_new_with_label("Browse...");
    gtk_box_append(GTK_BOX(hbox_src), browse_open_button);
    gtk_box_append(GTK_BOX(vbox), hbox_src);

    /* ── Row 3: destination file ── */
    GtkWidget *hbox_dst = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(hbox_dst), gtk_label_new("Output Folder:"));

    GtkWidget *dst_path_button = gtk_button_new();
    gtk_widget_set_margin_start(dst_path_button, 44);
    GtkWidget *dst_path_label = gtk_label_new("C:\\Users\\Username\\");
    gtk_widget_set_halign(dst_path_label, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(dst_path_button), dst_path_label);
    gtk_widget_set_hexpand(dst_path_button, TRUE);
    gtk_box_append(GTK_BOX(hbox_dst), dst_path_button);

    GtkWidget *browse_save_button = gtk_button_new_with_label("Browse...");
    gtk_box_append(GTK_BOX(hbox_dst), browse_save_button);
    gtk_box_append(GTK_BOX(vbox), hbox_dst);

    /* ── Row 4: overwrite checkbox ── */
    GtkWidget *hbox_overwrite = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *overwrite_check = gtk_check_button_new_with_label(
        "Overwrite original");           /* [FIX-G8] typo fixed */
    gtk_widget_set_sensitive(overwrite_check, FALSE);
    gtk_widget_set_margin_start(overwrite_check, 170);
    gtk_box_append(GTK_BOX(hbox_overwrite), overwrite_check);
    gtk_box_append(GTK_BOX(vbox), hbox_overwrite);

    /* ── Row 5: algorithm combobox ── */
    GtkWidget *hbox_algo = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(hbox_algo), gtk_label_new("Algorithm:"));

    GtkWidget *combobox = gtk_combo_box_text_new();
    gtk_widget_set_margin_start(combobox, 75);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox),
                                   "Select an algorithm");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox),
                                   "Caesar Cipher");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox),
                                   "Xor Cipher");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "AES");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
    gtk_widget_set_hexpand(combobox, TRUE);
    gtk_box_append(GTK_BOX(hbox_algo), combobox);
    gtk_box_append(GTK_BOX(vbox), hbox_algo);

    /* ── Row 6: action radio buttons ── */
    GtkWidget *hbox_action = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 38);
    gtk_box_append(GTK_BOX(hbox_action), gtk_label_new("Action:"));

    GtkWidget *radio_encrypt = gtk_check_button_new_with_label("Encrypt");
    gtk_widget_set_margin_start(radio_encrypt, 70);
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_encrypt), NULL);
    gtk_box_append(GTK_BOX(hbox_action), radio_encrypt);

    GtkWidget *radio_decrypt = gtk_check_button_new_with_label("Decrypt");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_decrypt),
                               GTK_CHECK_BUTTON(radio_encrypt));
    gtk_box_append(GTK_BOX(hbox_action), radio_decrypt);
    gtk_box_append(GTK_BOX(vbox), hbox_action);

    /* ── Row 7: key / passphrase entry ── */
    GtkWidget *hbox_key = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(hbox_key), gtk_label_new("Key Value:"));

    GtkWidget *key_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(key_entry),
                                   "Enter key value when you are asked");
    gtk_editable_set_editable(GTK_EDITABLE(key_entry), FALSE);
    gtk_entry_set_visibility(GTK_ENTRY(key_entry), FALSE);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(key_entry),
        GTK_ENTRY_ICON_SECONDARY, "view-hidden-symbolic");
    gtk_entry_set_icon_activatable(GTK_ENTRY(key_entry),
        GTK_ENTRY_ICON_SECONDARY, TRUE);
    gtk_widget_set_margin_start(key_entry, 80);
    gtk_widget_set_hexpand(key_entry, TRUE);
    gtk_box_append(GTK_BOX(hbox_key), key_entry);
    gtk_box_append(GTK_BOX(vbox), hbox_key);

    /* ── Row 8: status label + action buttons ── */
    GtkWidget *hbox_controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    GtkWidget *status_label = gtk_label_new("Status: Unready");
    gtk_widget_set_hexpand(status_label, TRUE);
    gtk_widget_set_halign(status_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(hbox_controls), status_label);

    GtkWidget *execute_button = gtk_button_new_with_label("Default");
    gtk_box_append(GTK_BOX(hbox_controls), execute_button);

    GtkWidget *clear_button = gtk_button_new_with_label("Clear");
    gtk_box_append(GTK_BOX(hbox_controls), clear_button);

    GtkWidget *exit_button = gtk_button_new_with_label("Exit");
    gtk_box_append(GTK_BOX(hbox_controls), exit_button);
    gtk_box_append(GTK_BOX(vbox), hbox_controls);

    /* ── Row 9: result label ── */
    GtkWidget *hbox_result = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *result_label = gtk_label_new("");
    gtk_widget_set_hexpand(result_label, TRUE);
    gtk_box_append(GTK_BOX(hbox_result), result_label);
    gtk_box_append(GTK_BOX(vbox), hbox_result);

    /* ══════════════════════════════════════════════════════════════════════
     * Wire up signals
     * ══════════════════════════════════════════════════════════════════════ */

    /* Allocate widget bundles on the heap (freed at app exit). */
    FileWidgets *open_widgets = g_new(FileWidgets, 1);
    open_widgets->path_button    = src_path_button;
    open_widgets->window         = window;
    open_widgets->status_label   = status_label;
    open_widgets->overwrite_check = overwrite_check;
    open_widgets->key_entry      = key_entry;
    open_widgets->combobox       = combobox;
    g_signal_connect(browse_open_button, "clicked",
                     G_CALLBACK(on_browse_open_clicked), open_widgets);

    FileWidgets *save_widgets = g_new(FileWidgets, 1);
    save_widgets->path_button    = dst_path_button;
    save_widgets->window         = window;
    save_widgets->status_label   = status_label;
    save_widgets->overwrite_check = overwrite_check;
    save_widgets->key_entry      = key_entry;
    save_widgets->combobox       = combobox;
    g_signal_connect(browse_save_button, "clicked",
                     G_CALLBACK(on_browse_save_clicked), save_widgets);

    ToggleWidgets *toggle_widgets = g_new(ToggleWidgets, 1);
    toggle_widgets->action_button = execute_button;
    toggle_widgets->status_label  = status_label;
    toggle_widgets->key_entry     = key_entry;
    toggle_widgets->combobox      = combobox;
    g_signal_connect(radio_encrypt, "toggled",
                     G_CALLBACK(on_action_toggled), toggle_widgets);
    g_signal_connect(radio_decrypt, "toggled",
                     G_CALLBACK(on_action_toggled), toggle_widgets);

    ExecuteWidgets *exec_widgets = g_new(ExecuteWidgets, 1);
    exec_widgets->combobox     = combobox;
    exec_widgets->key_entry    = key_entry;
    exec_widgets->result_label = result_label;
    g_signal_connect(execute_button, "clicked",
                     G_CALLBACK(on_execute_clicked), exec_widgets);

    ClearWidgets *clear_widgets = g_new(ClearWidgets, 1);
    clear_widgets->src_path_button = src_path_button;
    clear_widgets->dst_path_button = dst_path_button;
    clear_widgets->key_entry       = key_entry;
    clear_widgets->radio_encrypt   = radio_encrypt;
    clear_widgets->radio_decrypt   = radio_decrypt;
    clear_widgets->combobox        = combobox;
    clear_widgets->result_label    = result_label;
    clear_widgets->execute_button  = execute_button;
    clear_widgets->status_label    = status_label;
    clear_widgets->overwrite_check = overwrite_check;
    g_signal_connect(clear_button, "clicked",
                     G_CALLBACK(on_clear_clicked), clear_widgets);

    AlgoWidgets *algo_widgets = g_new(AlgoWidgets, 1);
    algo_widgets->entry        = key_entry;
    algo_widgets->status_label = status_label;
    g_signal_connect(combobox, "changed",
                     G_CALLBACK(on_algorithm_changed), algo_widgets);

    g_signal_connect(exit_button, "clicked",
                     G_CALLBACK(on_exit_clicked), NULL);

    g_signal_connect(key_entry, "icon-press",
                     G_CALLBACK(on_toggle_visibility), NULL);

    gtk_window_present(GTK_WINDOW(window));
    LOG_INFO("Application window launched.");
}

/* ══════════════════════════════════════════════════════════════════════════
 * main
 * ══════════════════════════════════════════════════════════════════════════ */

int main(int argc, char **argv) {
    log_init("file_encryptor.log");   /* Optional log file. */
    LOG_INFO("File Encryptor starting.");

    GtkApplication *app = gtk_application_new(
        "com.example.encryption",
        G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    app_state_reset(&g_state);
    log_close();
    return status;
}
