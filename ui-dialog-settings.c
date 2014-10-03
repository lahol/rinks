/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#include "ui-dialog-settings.h"
#include "ui.h"
#include "tournament.h"
#include <glib/gprintf.h>
#include "application.h"

GtkWidget *ui_dialog_settings = NULL;

struct UiDialogSettingsEntry {
    gchar *key;
    GtkWidget *entry;
};

GList *ui_dialog_settings_entries = NULL; /* [element-type: struct UiDialogSettingsEntry] */

GtkWidget *ui_dialog_settings_init_entries(void);
void ui_dialog_settings_write_data(void);
void ui_dialog_settings_read_data(void);

void ui_dialog_settings_free_settings_entry(struct UiDialogSettingsEntry *entry)
{
    if (entry == NULL)
        return;

    g_free(entry->key);
    if (GTK_IS_WIDGET(entry->entry))
        gtk_widget_destroy(entry->entry);

    g_free(entry);
}

void ui_dialog_settings_free_entries(GList *entries)
{
    g_list_free_full(entries, (GDestroyNotify)ui_dialog_settings_free_settings_entry);
}

void ui_dialog_settings_apply_cb(gpointer data)
{
    g_printf("ui-dialog-settings: apply\n");
    ui_dialog_settings_read_data();

    tournament_write_data(application_get_current_tournament());
}

void ui_dialog_settings_update_view_cb(gpointer data)
{
    g_printf("ui-dialog-settings: update view\n");

    ui_dialog_settings_write_data();
}

void ui_dialog_settings_activated_cb(gpointer data)
{
    ui_dialog_settings_write_data();
}

void ui_dialog_settings_destroy_cb(gpointer data)
{
    g_printf("ui-dialog-settings: destroy\n");
    ui_dialog_settings_free_entries(ui_dialog_settings_entries);
    ui_dialog_settings_entries = NULL;
}

GtkWidget *ui_dialog_settings_open(gpointer data)
{
    static UiDialogCallbacks callbacks = {
        .apply_cb = ui_dialog_settings_apply_cb,
        .destroy_cb = ui_dialog_settings_destroy_cb,
        .update_cb = ui_dialog_settings_update_view_cb,
        .activated_cb = ui_dialog_settings_activated_cb
    };
    if (!GTK_IS_WIDGET(ui_dialog_settings)) {
        ui_dialog_settings = ui_dialog_page_new("Einstellungen", &callbacks);
       
        GtkWidget *entries = ui_dialog_settings_init_entries();
        gtk_box_pack_start(GTK_BOX(ui_dialog_settings), entries, FALSE, FALSE, 0);
    }

    ui_dialog_settings_write_data();

    /* fill with data */

    gtk_widget_show_all(ui_dialog_settings);

    return ui_dialog_settings;
}

void ui_dialog_settings_add_settings_entry(const gchar *key, const gchar *label,
        GtkWidget *table, gint offset)
{
    GtkWidget *label_widget = gtk_label_new(label);
    GtkWidget *entry_widget = gtk_entry_new();

    struct UiDialogSettingsEntry *entry = g_malloc(sizeof(struct UiDialogSettingsEntry));
    entry->key = g_strdup(key);
    entry->entry = entry_widget;

    ui_dialog_settings_entries = g_list_append(ui_dialog_settings_entries, entry);

    gtk_table_attach(GTK_TABLE(table), label_widget, 0, 1, offset, offset + 1, 0, 0, 2, 2);
    gtk_table_attach(GTK_TABLE(table), entry_widget, 1, 2, offset, offset + 1, GTK_EXPAND | GTK_FILL, 0, 2, 2);
}

GtkWidget *ui_dialog_settings_init_entries(void)
{
    GtkWidget *table = gtk_table_new(3, 2, FALSE); /* rows, cols, homogenous */

    ui_dialog_settings_add_settings_entry("tournament.description", "Turniertitel:",
            table, 0);
    ui_dialog_settings_add_settings_entry("tournament.rink-count", "Anzahl Rinks:",
            table, 1);
    ui_dialog_settings_add_settings_entry("tournament.group-count", "Anzahl Gruppen:",
            table, 2);

    return table;
}

void ui_dialog_settings_write_data(void)
{
    GList *tmp;
    struct UiDialogSettingsEntry *entry;
    gchar *value;

    RinksTournament *tournament = application_get_current_tournament();

    for (tmp = ui_dialog_settings_entries; tmp != NULL; tmp = g_list_next(tmp)) {
        entry = (struct UiDialogSettingsEntry *)tmp->data;
        if (tournament != NULL)
            value = tournament_get_property(tournament, entry->key);
        else
            value = NULL;
        gtk_entry_set_text(GTK_ENTRY(entry->entry), value ? value : "");
        g_free(value);
    }
}

void ui_dialog_settings_read_data(void)
{
    GList *tmp;
    struct UiDialogSettingsEntry *entry;
    const gchar *value;

    RinksTournament *tournament = application_get_current_tournament();

    for (tmp = ui_dialog_settings_entries; tmp != NULL; tmp = g_list_next(tmp)) {
        entry = (struct UiDialogSettingsEntry *)tmp->data;
        value = gtk_entry_get_text(GTK_ENTRY(entry->entry));
        tournament_set_property(tournament, entry->key, value);
    }
}
