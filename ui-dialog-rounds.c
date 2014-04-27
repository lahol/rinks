#include <glib/gprintf.h>

#include "ui-dialog-rounds.h"
#include "ui.h"

#include "data.h"

#include "tournament.h"
#include "teams.h"
#include "rounds.h"
#include "application.h"

GtkWidget *ui_dialog_rounds = NULL;
GtkWidget *ui_dialog_rounds_table = NULL;

struct UiDialogRoundsEntry {
    gint64 round_id;
    GtkWidget *round_description;
    GtkWidget *round_type;
    GtkWidget *round_start;
    GtkWidget *round_end;
};

GList *ui_dialog_rounds_entries = NULL; /* [element-type: struct UiDialogRoundsEntry] */

void ui_dialog_rounds_create_entries(void);
void ui_dialog_rounds_add_entry(GtkWidget *table, gint offset, RinksRound *round, gint nteams);
void ui_dialog_rounds_clear_entries(void);
void ui_dialog_rounds_rebuild_entries(void);

void ui_dialog_rounds_apply_cb(gpointer data)
{
    g_printf("dialog-rounds: apply\n");
    
    GList *tmp;
    RinksRound round;
    struct UiDialogRoundsEntry *entry;
    RinksTournament *tournament = application_get_current_tournament();

    for (tmp = ui_dialog_rounds_entries; tmp != NULL; tmp = g_list_next(tmp)) {
        entry = (struct UiDialogRoundsEntry *)tmp->data;
        if (entry == NULL)
            continue;
        round.id = entry->round_id;
        round.description = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry->round_description)));
        round.type = gtk_combo_box_get_active(GTK_COMBO_BOX(entry->round_type));
        round.range_start = gtk_combo_box_get_active(GTK_COMBO_BOX(entry->round_start)) + 1;
        round.range_end = gtk_combo_box_get_active(GTK_COMBO_BOX(entry->round_end)) + 1;

        tournament_update_round(tournament, &round);
        g_free(round.description);
    }
}

void ui_dialog_rounds_update_cb(gpointer data)
{
    g_printf("dialog-rounds: update\n");
}

void ui_dialog_rounds_destroy_cb(gpointer data)
{
    g_printf("dialog-rounds: destroy\n");
    ui_dialog_rounds_clear_entries();
}

void ui_dialog_rounds_activated_cb(gpointer data)
{
    g_printf("dialog-rounds: activated\n");
}

void ui_dialog_rounds_deactivated_cb(gpointer data)
{
    g_printf("dialog-rounds: deactivated\n");
}

void ui_dialog_rounds_data_changed_cb(gpointer data)
{
    g_printf("dialog-rounds: data changed\n");
    ui_dialog_rounds_rebuild_entries();
}

void ui_dialog_rounds_button_add_round_clicked(GtkButton *button, gpointer data)
{
    RinksTournament *tournament = application_get_current_tournament();
    gint nteams = tournament_get_team_count(tournament);
    RinksRound *round = g_malloc0(sizeof(RinksRound));

    round->description = g_strdup("Rundenbezeichnung");
    /* TODO: special value, meaning all teams */
    round->range_start = 1;
    round->range_end = nteams;
    round->type = ROUND_TYPE_NEXT_TO_GROUP;
    
    round->id = tournament_add_round(tournament, round);

    guint rows = 0;

    if (ui_dialog_rounds_table == NULL) {
        ui_dialog_rounds_table = gtk_table_new(1, 5, FALSE);

        gtk_box_pack_start(GTK_BOX(ui_dialog_rounds), ui_dialog_rounds_table, FALSE, FALSE, 0);
    }
    else {
        gtk_table_get_size(GTK_TABLE(ui_dialog_rounds_table), &rows, NULL);
        gtk_table_resize(GTK_TABLE(ui_dialog_rounds_table), rows + 1, 5);
    }

    ui_dialog_rounds_add_entry(ui_dialog_rounds_table, rows, round, nteams);

    gtk_widget_show_all(ui_dialog_rounds_table);
}

void ui_dialog_rounds_create_entries(void)
{
    RinksTournament *tournament = application_get_current_tournament();
    gint nteams = tournament_get_team_count(tournament);
    GList *rounds = tournament_get_rounds(tournament);

    GList *tmp;
    gint nrounds = g_list_length(rounds);

    g_printf("ui-dialog-rounds: create entries: nrounds: %d\n", nrounds);

    gint offset;

    if (nrounds > 0) {
        ui_dialog_rounds_table = gtk_table_new(nrounds, 5, FALSE);

        for (tmp = rounds, offset = 0; tmp != NULL; tmp = g_list_next(tmp), ++offset) {
            ui_dialog_rounds_add_entry(ui_dialog_rounds_table, offset, (RinksRound *)tmp->data, nteams);
        }

        gtk_box_pack_start(GTK_BOX(ui_dialog_rounds), ui_dialog_rounds_table, FALSE, FALSE, 0);

        gtk_widget_show_all(ui_dialog_rounds_table);
    }
    else {
        ui_dialog_rounds_table = NULL;
    }

    g_list_free_full(rounds, (GDestroyNotify)round_free);
}

void ui_dialog_rounds_init_type_widget(GtkWidget *combobox, gint selection)
{
    g_return_if_fail(combobox != NULL);

    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(combobox), ROUND_TYPE_NEXT_TO_GROUP,
            "Nachbar in Gruppe");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(combobox), ROUND_TYPE_NEXT_TO_ALL,
            "Nachbar in Gesamtrangliste");
/*    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(combobox), ROUND_TYPE_ROUND_ROBIN,
            "Round Robin");*/

    if (selection >= 0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), selection);
    }
}

void ui_dialog_rounds_init_range_widget(GtkWidget *combobox, gint teamcount, gint selection)
{
    g_return_if_fail(combobox != NULL);

    gchar *text;
    gint i;

    for (i = 0; i < teamcount; ++i) {
        text = g_strdup_printf("%d", (i + 1));
        gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(combobox), i, text);
        g_free(text);
    }

    if (selection > 0 && selection <= teamcount) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), selection - 1);
    }
}

void ui_dialog_rounds_add_entry(GtkWidget *table, gint offset, RinksRound *round, gint nteams)
{
    g_return_if_fail(round != NULL);
    g_return_if_fail(GTK_IS_WIDGET(table));

    struct UiDialogRoundsEntry *entry = g_malloc0(sizeof(struct UiDialogRoundsEntry));
    entry->round_id = round->id;

    gchar *text = g_strdup_printf("Runde %" G_GINT64_FORMAT ":", round->id);
    GtkWidget *label = gtk_label_new(text);
    g_free(text);

    gtk_table_attach(GTK_TABLE(table), label, 0, 1, offset, offset + 1, 0, 0, 2, 2);

    entry->round_description = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry->round_description), round->description ? round->description : "");
    gtk_table_attach(GTK_TABLE(table), entry->round_description, 1, 2, offset, offset + 1, 0, 0, 2, 2);

    entry->round_type = gtk_combo_box_text_new();
    ui_dialog_rounds_init_type_widget(entry->round_type, round->type);
    gtk_table_attach(GTK_TABLE(table), entry->round_type, 2, 3, offset, offset + 1, 0, 0, 2, 2);
    
    entry->round_start = gtk_combo_box_text_new();
    ui_dialog_rounds_init_range_widget(entry->round_start, nteams, round->range_start);
    gtk_table_attach(GTK_TABLE(table), entry->round_start, 3, 4, offset, offset + 1, 0, 0, 2, 2);

    entry->round_end = gtk_combo_box_text_new();
    ui_dialog_rounds_init_range_widget(entry->round_end, nteams, round->range_end);
    gtk_table_attach(GTK_TABLE(table), entry->round_end, 4, 5, offset, offset + 1, 0, 0, 2, 2);

    ui_dialog_rounds_entries = g_list_append(ui_dialog_rounds_entries, entry);
}

void ui_dialog_rounds_clear_entries(void)
{
    if (ui_dialog_rounds_table != NULL)
        gtk_widget_destroy(ui_dialog_rounds_table);

    ui_dialog_rounds_table = NULL;

    g_list_free_full(ui_dialog_rounds_entries, g_free);
    ui_dialog_rounds_entries = NULL;
}

void ui_dialog_rounds_rebuild_entries(void)
{
    ui_dialog_rounds_clear_entries();
    ui_dialog_rounds_create_entries();
}

GtkWidget *ui_dialog_rounds_open(gpointer data)
{
    static UiDialogCallbacks callbacks = {
        .apply_cb = ui_dialog_rounds_apply_cb,
        .update_cb = ui_dialog_rounds_update_cb,
        .destroy_cb = ui_dialog_rounds_destroy_cb,
        .activated_cb = ui_dialog_rounds_activated_cb,
        .deactivated_cb = ui_dialog_rounds_deactivated_cb,
        .data_changed_cb = ui_dialog_rounds_data_changed_cb
    };

    GtkWidget *button;

    if (!GTK_IS_WIDGET(ui_dialog_rounds)) {
        ui_dialog_rounds = ui_dialog_page_new("Runden", &callbacks);

        button = gtk_button_new_with_label("Neue Runde");
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(ui_dialog_rounds_button_add_round_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(ui_dialog_rounds), button, FALSE, FALSE, 3);
    }

    gtk_widget_show_all(ui_dialog_rounds);

    return ui_dialog_rounds;
}
