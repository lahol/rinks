#pragma once

#include <gtk/gtk.h>

GtkWidget *ui_create_main_window(void);

void ui_set_action_callback(void (*callback)(gchar *action));

gchar *ui_get_filename(GtkFileChooserAction action);

typedef enum {
    VIEW_SETTINGS,
    VIEW_TEAMS
} UiViewType;

typedef struct {
    void (*apply_cb)(gpointer);
    void (*destroy_cb)(gpointer);
    void (*update_cb)(gpointer);
    void (*activated_cb)(gpointer);
    void (*deactivated_cb)(gpointer);
    void (*data_changed_cb)(gpointer);
} UiDialogCallbacks;

typedef enum {
    CB_apply,
    CB_destroy,
    CB_update,
    CB_activated,
    CB_deactivated,
    CB_data_changed
} UiDialogCallbackType;

void ui_select_view(UiViewType type, gpointer data);
void ui_update_view(void);
void ui_data_changed(void);
void ui_cleanup(void);

GtkWidget *ui_dialog_page_new(gchar *title, UiDialogCallbacks *callbacks);
