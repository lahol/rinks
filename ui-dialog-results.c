#include <glib/gprintf.h>
#include <stdlib.h>

#include "ui-dialog-results.h"
#include "ui.h"

#include "tournament.h"
#include "encounters.h"
#include "teams.h"
#include "application.h"

GtkWidget *ui_dialog_results = NULL;

UiDialogPageData *page_data = NULL;

struct UiDialogResultsEntry {
    gint64 encounter_id;
    gint64 teams[2];
    GtkWidget *points[2];
    GtkWidget *ends[2];
    GtkWidget *stones[2];
};

GList *ui_dialog_results_entries = NULL;
GtkWidget *ui_dialog_results_table = NULL;

void ui_dialog_results_clear_entries(void)
{
    g_printf("dialog-results: clear entries\n");

    gtk_widget_destroy(ui_dialog_results_table);
    ui_dialog_results_table = NULL;

    g_list_free_full(ui_dialog_results_entries, g_free);
    ui_dialog_results_entries = NULL;
}

void ui_dialog_results_add_entry(GtkWidget *table, gint offset,
        RinksTournament *tournament, RinksEncounter *encounter)
{
    g_printf("dialog-results: add entry\n");
    struct UiDialogResultsEntry *entry = g_malloc(sizeof(struct UiDialogResultsEntry));
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
        gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, offset + i, offset + i + 1, GTK_FILL, 0, 2, 2);

        entry->points[i] = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(entry->points[i]), 2);
        gtk_table_attach(GTK_TABLE(table), entry->points[i], 1, 2, offset + i, offset + i + 1, 0, 0, 2, 2);

        entry->ends[i] = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(entry->ends[i]), 3);
        gtk_table_attach(GTK_TABLE(table), entry->ends[i], 2, 3, offset + i, offset + i + 1, 0, 0, 2, 2);

        entry->stones[i] = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(entry->stones[i]), 3);
        gtk_table_attach(GTK_TABLE(table), entry->stones[i], 3, 4, offset + i, offset + i + 1, 0, 0, 2, 2);

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

    gtk_table_set_row_spacing(GTK_TABLE(table), offset + 1, 15);

    ui_dialog_results_entries = g_list_prepend(ui_dialog_results_entries, entry);
}

void ui_dialog_results_create_entries(void)
{
    g_printf("dialog-results: create entries (page data: %p)\n", page_data);
    if (page_data == NULL)
        return;
    RinksTournament *tournament = application_get_current_tournament();
    if (tournament == NULL)
        return;

    GList *encounters = tournament_get_encounters(tournament, 0, page_data->id);
    GList *tmp;

    guint nencounters = g_list_length(encounters);
    gint offset;

    if (encounters == 0)
        return;
    ui_dialog_results_table = gtk_table_new(2*nencounters + 1, 4, FALSE);

    GtkWidget *label;
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Team</b>");
    gtk_table_attach(GTK_TABLE(ui_dialog_results_table), label, 0, 1, 0, 1, GTK_FILL, 0, 2, 2);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Punkte</b>");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(ui_dialog_results_table), label, 1, 2, 0, 1, GTK_FILL, 0, 2, 2);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Ends</b>");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(ui_dialog_results_table), label, 2, 3, 0, 1, GTK_FILL, 0, 2, 2);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Steine</b>");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(ui_dialog_results_table), label, 3, 4, 0, 1, GTK_FILL, 0, 2, 2);
    for (tmp = encounters, offset = 1; tmp != NULL; tmp = g_list_next(tmp), offset += 2) {
        ui_dialog_results_add_entry(ui_dialog_results_table, offset,
                tournament, (RinksEncounter *)tmp->data);
    }

    gtk_widget_show_all(ui_dialog_results_table);
    gtk_box_pack_start(GTK_BOX(ui_dialog_results), ui_dialog_results_table, FALSE, FALSE, 3);

    g_list_free_full(encounters, (GDestroyNotify)encounter_free);
}

void ui_dialog_results_rebuild(void)
{
    g_printf("dialog-results: rebuild\n");
    ui_dialog_results_clear_entries();
    ui_dialog_results_create_entries();
}

void ui_dialog_results_apply_cb(gpointer data)
{
    g_printf("dialog-results: apply\n");

    GList *tmp;
    RinksResult result;
    const gchar *value;
    struct UiDialogResultsEntry *entry;
    int i;

    RinksTournament *tournament = application_get_current_tournament();

    if (page_data == NULL)
        return;

    for (tmp = ui_dialog_results_entries; tmp != NULL; tmp = g_list_next(tmp)) {
        entry = (struct UiDialogResultsEntry *)tmp->data;
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
    rounds_update_encounters();
}

void ui_dialog_results_activate_cb(gpointer data)
{
    g_printf("dialog-results: activate: %p\n", data);
    page_data = (UiDialogPageData *)data;
    ui_dialog_results_rebuild();
}

void ui_dialog_results_deactivated_cb(gpointer data)
{
    g_printf("dialog-results: deactivate\n");
    page_data = NULL;
}

void ui_dialog_results_destroy_cb(gpointer data)
{
    g_printf("dialog-results: destroy\n");
    ui_dialog_results_clear_entries();
}

GtkWidget *ui_dialog_results_open(gpointer data)
{
    static UiDialogCallbacks callbacks = {
        .apply_cb = ui_dialog_results_apply_cb,
        .activated_cb = ui_dialog_results_activate_cb,
        .deactivated_cb = ui_dialog_results_deactivated_cb,
        .destroy_cb = ui_dialog_results_destroy_cb
    };

    if (!GTK_IS_WIDGET(ui_dialog_results)) {
        ui_dialog_results = ui_dialog_page_new("Rundenüberblick", &callbacks);

    }

    gtk_widget_show_all(ui_dialog_results);

    return ui_dialog_results;
}
