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

void ui_dialog_settings_apply_cb(gpointer data)
{
    g_printf("ui-dialog-settings: apply\n");
    ui_dialog_settings_read_data();

    tournament_update_database(application_get_current_tournament());
}

GtkWidget *ui_dialog_settings_open(gpointer data)
{
    if (!GTK_IS_WIDGET(ui_dialog_settings)) {
        ui_dialog_settings = gtk_vbox_new(FALSE, 0);
        g_object_ref(G_OBJECT(ui_dialog_settings));

        g_object_set_data(G_OBJECT(ui_dialog_settings),
                "apply-callback", ui_dialog_settings_apply_cb);
   /* 
                "view-type", VIEW_SETTINGS,
                NULL);*/

        GtkWidget *label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), "<span size=\"xx-large\" weight=\"ultrabold\">Einstellungen</span>");

        gtk_box_pack_start(GTK_BOX(ui_dialog_settings), label, FALSE, FALSE, 0);

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
    GtkWidget *table = gtk_table_new(2, 2, FALSE); /* rows, cols, homogenous */

    ui_dialog_settings_add_settings_entry("tournament.description", "Turniertitel:",
            table, 0);
    ui_dialog_settings_add_settings_entry("tournament.rink-count", "Anzahl Rinks:",
            table, 1);

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
        value = tournament_get_property(tournament, entry->key);
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
