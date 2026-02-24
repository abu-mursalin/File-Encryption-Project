/*============================================================
 * File    : ui.h
 * Purpose : GTK UI construction and callback declarations.
 *           All widget creation and signal connections live
 *           in ui.c – this header exposes only what other
 *           modules need.
 *============================================================*/

#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "common.h"

/*
 * ui_activate – GTK "activate" callback.
 *   Builds the entire application window.
 *   Connect this to GApplication::activate.
 */
void ui_activate(GtkApplication *app, gpointer user_data);

#endif /* UI_H */
