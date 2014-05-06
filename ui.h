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

typedef struct {
    gint32 type;
    gint64 id;
} UiDialogPageData;

typedef enum {
    CB_apply,
    CB_destroy,
    CB_update,
    CB_activated,
    CB_deactivated,
    CB_data_changed
} UiDialogCallbackType;

typedef enum {
    SIDEBAR_TYPE_SETTINGS,
    SIDEBAR_TYPE_TEAMS,
    SIDEBAR_TYPE_ROUNDS,
    SIDEBAR_TYPE_GAMES,
    SIDEBAR_TYPE_ENCOUNTERS,
    SIDEBAR_TYPE_RESULTS,
    SIDEBAR_TYPE_ROUND_OVERVIEW,
    SIDEBAR_TYPE_OVERVIEW
} UiSidebarEntryType;

void ui_select_view(UiViewType type, gpointer data);
void ui_update_view(void);
void ui_data_changed(void);
void ui_cleanup(void);

void ui_add_page(UiSidebarEntryType type, const gchar *title, gpointer data);

GtkWidget *ui_dialog_page_new(gchar *title, UiDialogCallbacks *callbacks);
