#include <glib/gprintf.h>
#include <stdlib.h>

#include "ui-dialog-round-overview.h"
#include "ui.h"

#include "tournament.h"
#include "encounters.h"
#include "teams.h"
#include "application.h"

GtkWidget *ui_dialog_round_overview = NULL;

UiDialogPageData *page_data = NULL;

struct UiDialogRoundOverviewEntry {
    gint64 encounter_id;
    gint64 teams[2];
    GtkWidget *points[2];
    GtkWidget *ends[2];
    GtkWidget *stones[2];
};

GList *ui_dialog_round_overview_entries = NULL;
GtkWidget *ui_dialog_round_overview_table = NULL;

void ui_dialog_round_overview_clear_entries(void)
{
    g_printf("dialog-round-overview: clear entries\n");

    gtk_widget_destroy(ui_dialog_round_overview_table);
    ui_dialog_round_overview_table = NULL;

    g_list_free_full(ui_dialog_round_overview_entries, g_free);
    ui_dialog_round_overview_entries = NULL;
}

void ui_dialog_round_overview_add_entry(GtkWidget *table, gint offset,
        RinksTournament *tournament, RinksEncounter *encounter)
{
    g_printf("dialog-round-overview: add entry\n");
    struct UiDialogRoundOverviewEntry *entry = g_malloc(sizeof(struct UiDialogRoundOverviewEntry));
    entry->encounter_id = encounter->id;

    gint i;
    RinksTeam *team;
    RinksResult *result;
    GtkWidget *label;

    entry->teams[0] = encounter->real_team1;
    entry->teams[1] = encounter->real_team2;

    gchar *buffer;

    for (i = 0; i < 2; ++i) {
        team = tournament_get_team(tournament, i == 0 ? encounter->real_team1 : encounter->real_team2);
        if (team) {
            label = gtk_label_new(team->name); /* get team name */
            result = tournament_get_result(tournament, encounter->id, team->id);
        }
        else {
            label = gtk_label_new("Team nicht gesetzt");
            result = NULL;
        }
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2 * offset + i, 2 * offset + i + 1, 0, 0, 2, 2);

        entry->points[i] = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(entry->points[i]), 2);
        gtk_table_attach(GTK_TABLE(table), entry->points[i], 1, 2, 2 * offset + i, 2 * offset + i + 1, 0, 0, 2, 2);

        entry->ends[i] = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(entry->ends[i]), 3);
        gtk_table_attach(GTK_TABLE(table), entry->ends[i], 2, 3, 2 * offset + i, 2 * offset + i + 1, 0, 0, 2, 2);

        entry->stones[i] = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(entry->stones[i]), 3);
        gtk_table_attach(GTK_TABLE(table), entry->stones[i], 3, 4, 2 * offset + i, 2 * offset + i + 1, 0, 0, 2, 2);

        if (result) {
            buffer = g_strdup_printf("%d", result->points);
            gtk_entry_set_text(GTK_ENTRY(entry->points[i]), buffer);
            g_free(buffer);

            buffer = g_strdup_printf("%d", result->ends);
            gtk_entry_set_text(GTK_ENTRY(entry->ends[i]), buffer);
            g_free(buffer);

            buffer = g_strdup_printf("%d", result->stones);
            gtk_entry_set_text(GTK_ENTRY(entry->stones[i]), buffer);
            g_free(buffer);

            g_free(result);
        }
    }

    gtk_table_set_row_spacing(GTK_TABLE(table), 2 * offset + 1, 15);

    ui_dialog_round_overview_entries = g_list_prepend(ui_dialog_round_overview_entries, entry);
}

void ui_dialog_round_overview_create_entries(void)
{
    g_printf("dialog-round-overview: create entries (page data: %p)\n", page_data);
    if (page_data == NULL)
        return;
    RinksTournament *tournament = application_get_current_tournament();
    if (tournament == NULL)
        return;

    GList *encounters = tournament_get_encounters(tournament, page_data->id);
    GList *tmp;

    guint nencounters = g_list_length(encounters);
    gint offset;

    if (encounters == 0)
        return;
    ui_dialog_round_overview_table = gtk_table_new(2*nencounters, 4, FALSE);

    for (tmp = encounters, offset = 0; tmp != NULL; tmp = g_list_next(tmp), ++offset) {
        ui_dialog_round_overview_add_entry(ui_dialog_round_overview_table, offset,
                tournament, (RinksEncounter *)tmp->data);
    }

    gtk_widget_show_all(ui_dialog_round_overview_table);
    gtk_box_pack_start(GTK_BOX(ui_dialog_round_overview), ui_dialog_round_overview_table, FALSE, FALSE, 3);

    g_list_free_full(encounters, (GDestroyNotify)encounter_free);
}

void ui_dialog_round_overview_rebuild(void)
{
    g_printf("dialog-round-overview: rebuild\n");
    ui_dialog_round_overview_clear_entries();
    ui_dialog_round_overview_create_entries();
}

void ui_dialog_round_overview_apply_cb(gpointer data)
{
    g_printf("dialog-round-overview: apply\n");

    GList *tmp;
    RinksResult result;
    const gchar *value;
    struct UiDialogRoundOverviewEntry *entry;
    int i;

    RinksTournament *tournament = application_get_current_tournament();

    if (page_data == NULL)
        return;

    for (tmp = ui_dialog_round_overview_entries; tmp != NULL; tmp = g_list_next(tmp)) {
        entry = (struct UiDialogRoundOverviewEntry *)tmp->data;
        for (i = 0; i < 2; ++i) {
            if (entry->teams[i] != 0 && entry->encounter_id != 0) {
                result.id = 0;
                result.team = entry->teams[i];
                result.encounter = entry->encounter_id;

                value = gtk_entry_get_text(GTK_ENTRY(entry->points[i]));
                result.points = value ? atoi(value) : 0;
                
                value = gtk_entry_get_text(GTK_ENTRY(entry->ends[i]));
                result.ends = value ? atoi(value) : 0;

                value = gtk_entry_get_text(GTK_ENTRY(entry->stones[i]));
                result.stones = value ? atoi(value) : 0;

                tournament_set_result(tournament, &result);
            }
        }
    }

    tournament_update_standings(tournament);
}

void ui_dialog_round_overview_activate_cb(gpointer data)
{
    g_printf("dialog-round-overview: activate: %p\n", data);
    page_data = (UiDialogPageData *)data;
    ui_dialog_round_overview_rebuild();
}

void ui_dialog_round_overview_deactivated_cb(gpointer data)
{
    g_printf("dialog-round-overview: deactivate\n");
    page_data = NULL;
}

void ui_dialog_round_overview_destroy_cb(gpointer data)
{
    g_printf("dialog-round-overview: destroy\n");
    ui_dialog_round_overview_clear_entries();
}

void ui_dialog_round_overview_button_create_games_clicked(GtkButton *button, gpointer data)
{
    g_printf("clicked: page_data: %p\n", page_data);
    /* game order via data */
    if (page_data == NULL)
        return;
    rounds_create_games(page_data->id, RinksGameOrderGroupwise);
    ui_dialog_round_overview_rebuild();
}

GtkWidget *ui_dialog_round_overview_open(gpointer data)
{
    static UiDialogCallbacks callbacks = {
        .apply_cb = ui_dialog_round_overview_apply_cb,
        .activated_cb = ui_dialog_round_overview_activate_cb,
        .deactivated_cb = ui_dialog_round_overview_deactivated_cb,
        .destroy_cb = ui_dialog_round_overview_destroy_cb
    };

    GtkWidget *button;

    if (!GTK_IS_WIDGET(ui_dialog_round_overview)) {
        ui_dialog_round_overview = ui_dialog_page_new("Rundenüberblick", &callbacks);

        button = gtk_button_new_with_label("Spiele erzeugen");
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(ui_dialog_round_overview_button_create_games_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(ui_dialog_round_overview), button, FALSE, FALSE, 3);

/*        ui_dialog_round_overview_text = gtk_text_view_new();
        gtk_box_pack_start(GTK_BOX(ui_dialog_round_overview),
                ui_dialog_round_overview_text, TRUE, TRUE, 3);*/
    }

    gtk_widget_show_all(ui_dialog_round_overview);

    return ui_dialog_round_overview;
}
