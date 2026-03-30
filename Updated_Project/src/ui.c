/*============================================================
 * File    : ui.c
 * Purpose : GTK4 user interface  –  Windows / MinGW build.
 *
 * NOTE ON PREVIOUS VERSIONS:
 *   Earlier versions tried to use <gio/gio.h>, g_file_read(),
 *   G_IS_FILE_DESCRIPTOR_BASED, dup(), and fdopen() to work
 *   around the XDG Desktop Portal on Linux.
 *   None of that exists or is needed on Windows / MinGW.
 *   On Windows, GtkFileChooserNative uses the Win32 common
 *   file dialog and g_file_get_path() always returns a plain
 *   "C:\path\to\file.jpg" string that fopen() handles fine.
 *   This version compiles cleanly with MinGW32/MinGW64.
 *
 * BUGS FIXED (kept from previous iterations):
 *   Bug 1 – Files opened at browse time (broken for all types).
 *            Fix: store path only; open inside on_run_clicked().
 *   Bug 2 – GtkFileChooserNative never g_object_unref'd.
 *            Fix: unref at top of every response handler.
 *   Bug 3 – Ready flags reset before operation ran.
 *            Fix: reset only after successful run or Clear.
 *============================================================*/

#include "ui.h"
#include "common.h"
#include "logger.h"
#include "file_handler.h"
#include "crypto_dispatcher.h"

#include <string.h>

/* ── Callback context structs ───────────────────────────── */

typedef struct {
    GtkWidget  *path_button;
    GtkWidget  *parent_window;
    GtkWidget  *status_label;
    GtkWidget  *overwrite_check;
    GtkWidget  *key_entry;
    GtkWidget  *algo_combo;
    AppState   *app_state;
} BrowseContext;

typedef struct {
    GtkWidget  *action_button;
    GtkWidget  *status_label;
    GtkWidget  *key_entry;
    GtkWidget  *algo_combo;
    AppState   *app_state;
} ActionContext;

typedef struct {
    GtkWidget  *algo_combo;
    GtkWidget  *key_entry;
    GtkWidget  *result_label;
    GtkWidget  *status_label;
    AppState   *app_state;
} EncryptContext;

typedef struct {
    GtkWidget  *input_path_button;
    GtkWidget  *output_path_button;
    GtkWidget  *key_entry;
    GtkWidget  *radio_encrypt;
    GtkWidget  *radio_decrypt;
    GtkWidget  *algo_combo;
    GtkWidget  *result_label;
    GtkWidget  *action_button;
    GtkWidget  *status_label;
    GtkWidget  *overwrite_check;
    AppState   *app_state;
} ClearContext;

/* ── Helpers ────────────────────────────────────────────── */

static void ui_update_key_placeholder(GtkWidget *key_entry,
                                       GtkWidget *algo_combo)
{
    const char *algo =
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(algo_combo));

    if      (!algo)
        gtk_entry_set_placeholder_text(GTK_ENTRY(key_entry),
                                       "Enter key value :");
    else if (strcmp(algo, ALGO_CAESAR) == 0)
        gtk_entry_set_placeholder_text(GTK_ENTRY(key_entry),
                                       "Enter shift key (1 - 25) :");
    else if (strcmp(algo, ALGO_XOR) == 0)
        gtk_entry_set_placeholder_text(GTK_ENTRY(key_entry),
                                       "Enter XOR key (1 - 255) :");
    else if (strcmp(algo, ALGO_AES) == 0)
        gtk_entry_set_placeholder_text(GTK_ENTRY(key_entry),
                                       "Enter passphrase (AES-256-GCM) :");
    else
        gtk_entry_set_placeholder_text(GTK_ENTRY(key_entry),
                                       "Enter key value :");
}

static void ui_check_ready(AppState  *state,
                            GtkWidget *status_label,
                            GtkWidget *key_entry,
                            GtkWidget *algo_combo)
{
    if (!state->action_chosen || !state->input_ready || !state->output_ready)
        return;

    gtk_label_set_text(GTK_LABEL(status_label), "Status : Ready");
    gtk_editable_set_editable(GTK_EDITABLE(key_entry), TRUE);
    ui_update_key_placeholder(key_entry, algo_combo);
}

/* ── Callbacks ──────────────────────────────────────────── */

static void on_toggle_key_visibility(GtkEntry             *entry,
                                      GtkEntryIconPosition  pos,
                                      GdkEvent             *ev,
                                      gpointer              ud)
{
    (void)pos; (void)ev; (void)ud;
    gboolean v = gtk_entry_get_visibility(entry);
    gtk_entry_set_visibility(entry, !v);
    gtk_entry_set_icon_from_icon_name(entry, GTK_ENTRY_ICON_SECONDARY,
        v ? "view-hidden-symbolic" : "view-visible-symbolic");
}

static void on_algorithm_changed(GtkComboBox *combo, gpointer user_data)
{
    ActionContext *ctx = (ActionContext *)user_data;
    const char    *s   = gtk_label_get_text(GTK_LABEL(ctx->status_label));
    if (strcmp(s, "Status : Ready") != 0) return;
    ui_update_key_placeholder(ctx->key_entry, GTK_WIDGET(combo));
}

static void on_action_toggled(GtkWidget *radio, gpointer user_data)
{
    ActionContext *ctx = (ActionContext *)user_data;
    if (!gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) return;

    const char *label =
        gtk_check_button_get_label(GTK_CHECK_BUTTON(radio));
    ctx->app_state->action_label  = (char *)label;
    ctx->app_state->action_chosen = true;
    gtk_button_set_label(GTK_BUTTON(ctx->action_button), label);
    LOG_INFO("Action selected: %s", label);
    ui_check_ready(ctx->app_state, ctx->status_label,
                   ctx->key_entry, ctx->algo_combo);
}

/* ── on_response_open ───────────────────────────────────────
 * Store the file path only – do NOT open the file here.
 * On Windows g_file_get_path() always returns a valid
 * "C:\..." path.  Opening is done in on_run_clicked().
 * ─────────────────────────────────────────────────────────── */
static void on_response_open(GtkNativeDialog *native, int response,
                              gpointer user_data)
{
    g_object_unref(native);   /* Bug 2 fix: always unref the dialog */
    if (response != GTK_RESPONSE_ACCEPT) return;

    BrowseContext *ctx   = (BrowseContext *)user_data;
    AppState      *state = ctx->app_state;

    /* Release previous input handle/path */
    file_close_safe(&state->input_file);
    g_free(state->input_path);
    state->input_path = NULL;

    GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
    GFile          *gfile   = gtk_file_chooser_get_file(chooser);

    if (gfile == NULL) {
        LOG_ERROR("on_response_open: get_file returned NULL");
        gtk_label_set_text(GTK_LABEL(ctx->status_label),
                           "Status : Could not get selected file");
        return;
    }

    /*
     * On Windows, g_file_get_path() always succeeds and returns
     * a standard "C:\Users\...\photo.jpg" path.
     */
    state->input_path = g_file_get_path(gfile);
    g_object_unref(gfile);

    if (state->input_path == NULL) {
        LOG_ERROR("on_response_open: g_file_get_path returned NULL");
        gtk_label_set_text(GTK_LABEL(ctx->status_label),
                           "Status : Could not resolve file path");
        return;
    }

    LOG_INFO("Input path stored: %s", state->input_path);

    /* Update button label */
    GtkWidget *lbl = gtk_label_new(state->input_path);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(ctx->path_button), lbl);

    state->input_ready = true;

    if (state->output_path &&
        strcmp(state->input_path, state->output_path) == 0)
        gtk_check_button_set_active(
            GTK_CHECK_BUTTON(ctx->overwrite_check), TRUE);

    ui_check_ready(state, ctx->status_label,
                   ctx->key_entry, ctx->algo_combo);
}

/* ── on_response_save ─────────────────────────────────────── */
static void on_response_save(GtkNativeDialog *native, int response,
                              gpointer user_data)
{
    g_object_unref(native);   /* Bug 2 fix */
    if (response != GTK_RESPONSE_ACCEPT) return;

    BrowseContext *ctx   = (BrowseContext *)user_data;
    AppState      *state = ctx->app_state;

    file_close_safe(&state->output_file);
    g_free(state->output_path);
    state->output_path = NULL;

    GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
    GFile          *gfile   = gtk_file_chooser_get_file(chooser);

    if (gfile == NULL) {
        LOG_ERROR("on_response_save: get_file returned NULL");
        gtk_label_set_text(GTK_LABEL(ctx->status_label),
                           "Status : Could not get output path");
        return;
    }

    state->output_path = g_file_get_path(gfile);
    g_object_unref(gfile);

    if (state->output_path == NULL) {
        LOG_ERROR("on_response_save: g_file_get_path returned NULL");
        gtk_label_set_text(GTK_LABEL(ctx->status_label),
                           "Status : Could not resolve output path");
        return;
    }

    LOG_INFO("Output path stored: %s", state->output_path);

    GtkWidget *lbl = gtk_label_new(state->output_path);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(ctx->path_button), lbl);

    state->output_ready = true;

    if (state->input_path &&
        strcmp(state->output_path, state->input_path) == 0)
        gtk_check_button_set_active(
            GTK_CHECK_BUTTON(ctx->overwrite_check), TRUE);

    ui_check_ready(state, ctx->status_label,
                   ctx->key_entry, ctx->algo_combo);
}

static void on_browse_open_clicked(GtkWidget *btn, gpointer user_data)
{
    (void)btn;
    BrowseContext *ctx = (BrowseContext *)user_data;
    GtkFileChooserNative *native =
        gtk_file_chooser_native_new("Select Input File",
            GTK_WINDOW(ctx->parent_window),
            GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    g_signal_connect(native, "response",
                     G_CALLBACK(on_response_open), ctx);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
}

static void on_browse_save_clicked(GtkWidget *btn, gpointer user_data)
{
    (void)btn;
    BrowseContext *ctx = (BrowseContext *)user_data;
    GtkFileChooserNative *native =
        gtk_file_chooser_native_new("Select Output File",
            GTK_WINDOW(ctx->parent_window),
            GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    g_signal_connect(native, "response",
                     G_CALLBACK(on_response_save), ctx);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
}

/* ── on_run_clicked ─────────────────────────────────────────
 * Open BOTH files here, right before crypto_dispatch().
 * This guarantees position 0 every run for any file type.
 * ─────────────────────────────────────────────────────────── */
static void on_run_clicked(GtkWidget *btn, gpointer user_data)
{
    (void)btn;
    EncryptContext *ctx   = (EncryptContext *)user_data;
    AppState       *state = ctx->app_state;

    /* Validate passphrase */
    const char *passphrase =
        gtk_editable_get_text(GTK_EDITABLE(ctx->key_entry));
    if (strlen(passphrase) == 0) {
        gtk_label_set_text(GTK_LABEL(ctx->result_label),
                           "Please enter a key / passphrase.");
        return;
    }

    if (state->action_label == NULL) {
        gtk_label_set_text(GTK_LABEL(ctx->result_label),
                           "Please select Encrypt or Decrypt.");
        return;
    }

    const char *algo = gtk_combo_box_text_get_active_text(
                           GTK_COMBO_BOX_TEXT(ctx->algo_combo));
    if (algo == NULL || strcmp(algo, "Select an algorithm") == 0) {
        gtk_label_set_text(GTK_LABEL(ctx->result_label),
                           "Please select an algorithm.");
        return;
    }

    if (state->input_path == NULL) {
        gtk_label_set_text(GTK_LABEL(ctx->result_label),
                           "Please choose an input file.");
        return;
    }

    if (state->output_path == NULL) {
        gtk_label_set_text(GTK_LABEL(ctx->result_label),
                           "Please choose an output file.");
        return;
    }

    /* Open input fresh – position guaranteed at byte 0 */
    FILE *input_file = file_open_read(state->input_path);
    if (input_file == NULL) {
        LOG_ERROR("on_run_clicked: cannot open input: %s",
                  state->input_path);
        gtk_label_set_text(GTK_LABEL(ctx->result_label),
                           "Error: Cannot open input file.");
        gtk_label_set_text(GTK_LABEL(ctx->status_label),
                           "Status : Error opening input file");
        return;
    }

    /* Open output fresh */
    bool overwrite = (strcmp(state->input_path,
                             state->output_path) == 0);
    FILE *output_file = file_open_write(state->output_path,
                                        state->input_path,
                                        overwrite);
    if (output_file == NULL) {
        LOG_ERROR("on_run_clicked: cannot open output: %s",
                  state->output_path);
        file_close_safe(&input_file);
        gtk_label_set_text(GTK_LABEL(ctx->result_label),
                           "Error: Cannot open output file.");
        gtk_label_set_text(GTK_LABEL(ctx->status_label),
                           "Status : Error opening output file");
        return;
    }

    LOG_INFO("Running %s with %s", state->action_label, algo);

    CryptoResult result =
        crypto_dispatch(algo, state->action_label, passphrase,
                        input_file, output_file);

    /* Always close both handles right after the operation */
    file_close_safe(&input_file);
    file_close_safe(&output_file);

    gtk_label_set_text(GTK_LABEL(ctx->result_label),
                       crypto_result_message(result));

    if (result == CRYPTO_OK) {
        LOG_INFO("Operation succeeded: %s -> %s",
                 state->input_path, state->output_path);
        gtk_label_set_text(GTK_LABEL(ctx->status_label),
                           "Status : Done");
        state->action_label  = NULL;
        state->action_chosen = false;  /* Bug 3 fix */
        state->input_ready   = false;
        state->output_ready  = false;
    } else {
        LOG_ERROR("Operation failed: %s",
                  crypto_result_message(result));
        gtk_label_set_text(GTK_LABEL(ctx->status_label),
                           "Status : Failed");
    }
}

static void on_clear_clicked(GtkWidget *btn, gpointer user_data)
{
    (void)btn;
    ClearContext *ctx   = (ClearContext *)user_data;
    AppState     *state = ctx->app_state;

    file_close_safe(&state->input_file);
    file_close_safe(&state->output_file);
    g_free(state->input_path);
    g_free(state->output_path);
    state->input_path    = NULL;
    state->output_path   = NULL;
    state->action_label  = NULL;
    state->action_chosen = false;
    state->input_ready   = false;
    state->output_ready  = false;

    GtkWidget *li = gtk_label_new("(no file selected)");
    gtk_widget_set_halign(li, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(ctx->input_path_button), li);

    GtkWidget *lo = gtk_label_new("(no folder selected)");
    gtk_widget_set_halign(lo, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(ctx->output_path_button), lo);

    gtk_editable_set_editable(GTK_EDITABLE(ctx->key_entry), FALSE);
    gtk_editable_set_text(GTK_EDITABLE(ctx->key_entry), "");
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->key_entry),
                                   "Enter key value when prompted");
    gtk_check_button_set_active(
        GTK_CHECK_BUTTON(ctx->radio_encrypt), FALSE);
    gtk_check_button_set_active(
        GTK_CHECK_BUTTON(ctx->radio_decrypt), FALSE);
    gtk_check_button_set_active(
        GTK_CHECK_BUTTON(ctx->overwrite_check), FALSE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->algo_combo), 0);
    gtk_label_set_text(GTK_LABEL(ctx->result_label), "");
    gtk_button_set_label(GTK_BUTTON(ctx->action_button), "Default");
    gtk_label_set_text(GTK_LABEL(ctx->status_label), "Status : Unready");
    LOG_INFO("UI cleared");
}

static void on_exit_clicked(GtkWidget *btn, gpointer ud)
{
    (void)btn; (void)ud;
    LOG_INFO("Exit requested");
    exit(0);
}

/* ── Window builder ─────────────────────────────────────── */

void ui_activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    AppState *state = g_new0(AppState, 1);

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), APP_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(window), APP_WIDTH, APP_HEIGHT);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_window_set_child(GTK_WINDOW(window), vbox);
    gtk_widget_set_margin_top(vbox,    40);
    gtk_widget_set_margin_bottom(vbox, 40);
    gtk_widget_set_margin_start(vbox,  40);
    gtk_widget_set_margin_end(vbox,    40);

    /* Header */
    GtkWidget *hbox_hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(hbox_hdr),
        gtk_label_new("Select a file to encrypt / decrypt"));
    gtk_box_append(GTK_BOX(vbox), hbox_hdr);

    /* Input file row */
    GtkWidget *hbox_in = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(hbox_in), gtk_label_new("Input File :"));
    GtkWidget *input_path_button = gtk_button_new();
    gtk_widget_set_margin_start(input_path_button, 50);
    GtkWidget *ipd = gtk_label_new("(no file selected)");
    gtk_widget_set_halign(ipd, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(input_path_button), ipd);
    gtk_widget_set_hexpand(input_path_button, TRUE);
    gtk_box_append(GTK_BOX(hbox_in), input_path_button);
    GtkWidget *browse_open_btn = gtk_button_new_with_label("Browse...");
    gtk_box_append(GTK_BOX(hbox_in), browse_open_btn);
    gtk_box_append(GTK_BOX(vbox), hbox_in);

    /* Output folder row */
    GtkWidget *hbox_out = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(hbox_out), gtk_label_new("Output Folder :"));
    GtkWidget *output_path_button = gtk_button_new();
    gtk_widget_set_margin_start(output_path_button, 20);
    GtkWidget *opd = gtk_label_new("(no folder selected)");
    gtk_widget_set_halign(opd, GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(output_path_button), opd);
    gtk_widget_set_hexpand(output_path_button, TRUE);
    gtk_box_append(GTK_BOX(hbox_out), output_path_button);
    GtkWidget *browse_save_btn = gtk_button_new_with_label("Browse...");
    gtk_box_append(GTK_BOX(hbox_out), browse_save_btn);
    gtk_box_append(GTK_BOX(vbox), hbox_out);

    /* Overwrite checkbox */
    GtkWidget *hbox_ow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *overwrite_check =
        gtk_check_button_new_with_label("Overwrite original");
    gtk_widget_set_sensitive(overwrite_check, FALSE);
    gtk_widget_set_margin_start(overwrite_check, 170);
    gtk_box_append(GTK_BOX(hbox_ow), overwrite_check);
    gtk_box_append(GTK_BOX(vbox), hbox_ow);

    /* Algorithm combo */
    GtkWidget *hbox_algo = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(hbox_algo), gtk_label_new("Algorithm :"));
    GtkWidget *algo_combo = gtk_combo_box_text_new();
    gtk_widget_set_margin_start(algo_combo, 55);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algo_combo),
                                   "Select an algorithm");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algo_combo),
                                   ALGO_CAESAR);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algo_combo),
                                   ALGO_XOR);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algo_combo),
                                   ALGO_AES);
    gtk_combo_box_set_active(GTK_COMBO_BOX(algo_combo), 0);
    gtk_widget_set_hexpand(algo_combo, TRUE);
    gtk_box_append(GTK_BOX(hbox_algo), algo_combo);
    gtk_box_append(GTK_BOX(vbox), hbox_algo);

    /* Radio buttons */
    GtkWidget *hbox_act = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 38);
    gtk_box_append(GTK_BOX(hbox_act), gtk_label_new("Action :"));
    GtkWidget *radio_encrypt =
        gtk_check_button_new_with_label("Encrypt");
    gtk_widget_set_margin_start(radio_encrypt, 70);
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_encrypt), NULL);
    gtk_box_append(GTK_BOX(hbox_act), radio_encrypt);
    GtkWidget *radio_decrypt =
        gtk_check_button_new_with_label("Decrypt");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_decrypt),
                               GTK_CHECK_BUTTON(radio_encrypt));
    gtk_box_append(GTK_BOX(hbox_act), radio_decrypt);
    gtk_box_append(GTK_BOX(vbox), hbox_act);

    /* Key entry */
    GtkWidget *hbox_key = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(hbox_key), gtk_label_new("Key Value :"));
    GtkWidget *key_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(key_entry),
                                   "Enter key value when prompted");
    gtk_editable_set_editable(GTK_EDITABLE(key_entry), FALSE);
    gtk_entry_set_visibility(GTK_ENTRY(key_entry), FALSE);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(key_entry),
        GTK_ENTRY_ICON_SECONDARY, "view-hidden-symbolic");
    gtk_entry_set_icon_activatable(GTK_ENTRY(key_entry),
        GTK_ENTRY_ICON_SECONDARY, TRUE);
    gtk_widget_set_margin_start(key_entry, 68);
    gtk_widget_set_hexpand(key_entry, TRUE);
    gtk_box_append(GTK_BOX(hbox_key), key_entry);
    gtk_box_append(GTK_BOX(vbox), hbox_key);

    /* Controls row */
    GtkWidget *hbox_ctrl = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *status_label = gtk_label_new("Status : Unready");
    gtk_widget_set_hexpand(status_label, TRUE);
    gtk_widget_set_halign(status_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(hbox_ctrl), status_label);
    GtkWidget *run_button   = gtk_button_new_with_label("Default");
    GtkWidget *clear_button = gtk_button_new_with_label("Clear");
    GtkWidget *exit_button  = gtk_button_new_with_label("Exit");
    gtk_box_append(GTK_BOX(hbox_ctrl), run_button);
    gtk_box_append(GTK_BOX(hbox_ctrl), clear_button);
    gtk_box_append(GTK_BOX(hbox_ctrl), exit_button);
    gtk_box_append(GTK_BOX(vbox), hbox_ctrl);

    /* Result label */
    GtkWidget *hbox_res = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *result_label = gtk_label_new("");
    gtk_widget_set_hexpand(result_label, TRUE);
    gtk_box_append(GTK_BOX(hbox_res), result_label);
    gtk_box_append(GTK_BOX(vbox), hbox_res);

    /* ── Wire signals ───────────────────────────────────── */

    BrowseContext *boc = g_new0(BrowseContext, 1);
    boc->path_button     = input_path_button;
    boc->parent_window   = window;
    boc->status_label    = status_label;
    boc->overwrite_check = overwrite_check;
    boc->key_entry       = key_entry;
    boc->algo_combo      = algo_combo;
    boc->app_state       = state;
    g_signal_connect(browse_open_btn, "clicked",
                     G_CALLBACK(on_browse_open_clicked), boc);

    BrowseContext *bsc = g_new0(BrowseContext, 1);
    bsc->path_button     = output_path_button;
    bsc->parent_window   = window;
    bsc->status_label    = status_label;
    bsc->overwrite_check = overwrite_check;
    bsc->key_entry       = key_entry;
    bsc->algo_combo      = algo_combo;
    bsc->app_state       = state;
    g_signal_connect(browse_save_btn, "clicked",
                     G_CALLBACK(on_browse_save_clicked), bsc);

    ActionContext *ac = g_new0(ActionContext, 1);
    ac->action_button = run_button;
    ac->status_label  = status_label;
    ac->key_entry     = key_entry;
    ac->algo_combo    = algo_combo;
    ac->app_state     = state;
    g_signal_connect(radio_encrypt, "toggled",
                     G_CALLBACK(on_action_toggled), ac);
    g_signal_connect(radio_decrypt, "toggled",
                     G_CALLBACK(on_action_toggled), ac);
    g_signal_connect(algo_combo, "changed",
                     G_CALLBACK(on_algorithm_changed), ac);

    EncryptContext *ec = g_new0(EncryptContext, 1);
    ec->algo_combo   = algo_combo;
    ec->key_entry    = key_entry;
    ec->result_label = result_label;
    ec->status_label = status_label;
    ec->app_state    = state;
    g_signal_connect(run_button, "clicked",
                     G_CALLBACK(on_run_clicked), ec);

    ClearContext *cc = g_new0(ClearContext, 1);
    cc->input_path_button  = input_path_button;
    cc->output_path_button = output_path_button;
    cc->key_entry          = key_entry;
    cc->radio_encrypt      = radio_encrypt;
    cc->radio_decrypt      = radio_decrypt;
    cc->algo_combo         = algo_combo;
    cc->result_label       = result_label;
    cc->action_button      = run_button;
    cc->status_label       = status_label;
    cc->overwrite_check    = overwrite_check;
    cc->app_state          = state;
    g_signal_connect(clear_button, "clicked",
                     G_CALLBACK(on_clear_clicked), cc);

    g_signal_connect(exit_button, "clicked",
                     G_CALLBACK(on_exit_clicked), NULL);
    g_signal_connect(key_entry, "icon-press",
                     G_CALLBACK(on_toggle_key_visibility), NULL);

    gtk_window_present(GTK_WINDOW(window));
    LOG_INFO("Application window created and presented");
}
