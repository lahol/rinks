#pragma once

#include <gtk/gtk.h>

GtkWidget *ui_create_main_window(void);

void ui_set_action_callback(void (*callback)(gchar *action));
