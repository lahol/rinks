#pragma once

#include <gtk/gtk.h>

GtkWidget *ui_create_main_window(void);

void ui_set_action_callback(void (*callback)(gchar *action));

gchar *ui_get_filename(GtkFileChooserAction action);

typedef enum {
    VIEW_SETTINGS,
    VIEW_TEAMS
} UiViewType;

void ui_select_view(UiViewType type, gpointer data);
