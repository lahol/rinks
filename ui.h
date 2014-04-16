#pragma once

#include <gtk/gtk.h>

GtkWidget *ui_create_main_window(void);

void ui_set_action_callback(void (*callback)(gchar *action));

gchar *ui_get_filename(GtkFileChooserAction action);
