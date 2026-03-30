/*============================================================
 * File    : main.c
 * Purpose : Application entry point.
 *           Initialises the logger, creates the GtkApplication,
 *           connects the activate signal to ui_activate, and
 *           starts the GTK main loop.
 *============================================================*/

#include <gtk/gtk.h>
#include "common.h"
#include "logger.h"
#include "ui.h"

int main(int argc, char **argv)
{
    /* ── Start logger (writes to stderr + optional file) ─ */
    logger_init("file_encryption.log");
    LOG_INFO("=== File Encryption Application starting ===");
    LOG_INFO("AES mode: AES-256-GCM with random IV");

    /* ── Create GTK application ───────────────────────── */
    GtkApplication *app =
        gtk_application_new(APP_ID, G_APPLICATION_DEFAULT_FLAGS);

    /* ── Connect activate signal ──────────────────────── */
    g_signal_connect(app, "activate", G_CALLBACK(ui_activate), NULL);

    /* ── Run the GTK main loop ────────────────────────── */
    int exit_status = g_application_run(G_APPLICATION(app), argc, argv);

    /* ── Cleanup ──────────────────────────────────────── */
    g_object_unref(app);

    LOG_INFO("Application exiting with status %d", exit_status);
    logger_close();

    return exit_status;
}
