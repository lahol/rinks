/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#include <glib/gprintf.h>

#include "ui-dialog-overrides.h"
#include "ui.h"
#include "ui-helpers.h"

#include "application.h"
#include "tournament.h"
#include "encounters.h"
#include "teams.h"

GtkWidget *ui_dialog_overrides = NULL;
GtkWidget *ui_dialog_overrides_table = NULL;
GtkTreeModel *ui_dialog_overrides_model_encounters = NULL;
GtkTreeModel *ui_dialog_overrides_model_teams = NULL;

gboolean ui_dialog_overrides_models_built = FALSE;

struct UiDialogOverridesEntry {
    gint64 override_id;
    GtkWidget *encounter;
    GtkWidget *teams[2];
};

#define OVERRIDES_ENTRY_LENGTH 3

GList *ui_dialog_overrides_entries = NULL;

void ui_dialog_overrides_models_clear(void)
{
    ui_helper_clear_tree_model(ui_dialog_overrides_model_encounters);
    ui_helper_clear_tree_model(ui_dialog_overrides_model_teams);

    ui_dialog_overrides_models_built = FALSE;
}

void ui_dialog_overrides_models_build(GList *encounters, GList *teams)
{
    ui_helper_build_combo_tree_model(&ui_dialog_overrides_model_encounters,
            UiHelperModelTypeEncounters, encounters);
    ui_helper_build_combo_tree_model(&ui_dialog_overrides_model_teams,
            UiHelperModelTypeTeams, teams);

    ui_dialog_overrides_models_built = TRUE;
}

void ui_dialog_overrides_add_entry(RinksTournament *tournament, GtkWidget *table, gint offset, RinksOverride *override)
{
    g_return_if_fail(tournament != NULL);
    g_return_if_fail(override != NULL);

    struct UiDialogOverridesEntry *entry = g_malloc0(sizeof(struct UiDialogOverridesEntry));
    entry->override_id = override->id;
    GList *encounters = NULL, *teams = NULL;

    if (ui_dialog_overrides_model_encounters == NULL ||
            ui_dialog_overrides_model_teams == NULL ||
            !ui_dialog_overrides_models_built) {
        encounters = encounters_sort(tournament_get_encounters(tournament, 0, 0), RinksEncounterSortLogical);
        teams = tournament_get_teams(tournament);

        ui_dialog_overrides_models_build(encounters, teams);

        g_list_free_full(encounters, (GDestroyNotify)encounter_free);
        g_list_free_full(teams, (GDestroyNotify)team_free);
    }

    entry->encounter = ui_helper_combo_widget_build(GTK_TREE_MODEL(ui_dialog_overrides_model_encounters));
    entry->teams[0] = ui_helper_combo_widget_build(GTK_TREE_MODEL(ui_dialog_overrides_model_teams));
    entry->teams[1] = ui_helper_combo_widget_build(GTK_TREE_MODEL(ui_dialog_overrides_model_teams));

    gtk_table_attach(GTK_TABLE(table), entry->encounter, 0, 1, offset, offset + 1, 0, 0, 2, 2);
    gtk_table_attach(GTK_TABLE(table), entry->teams[0], 1, 2, offset, offset + 1, 0, 0, 2, 2);
    gtk_table_attach(GTK_TABLE(table), entry->teams[1], 2, 3, offset, offset + 1, 0, 0, 2, 2);

    ui_helper_combo_widget_set_selection(entry->encounter, override->encounter);
    ui_helper_combo_widget_set_selection(entry->teams[0], override->team1);
    ui_helper_combo_widget_set_selection(entry->teams[1], override->team2);

    ui_dialog_overrides_entries = g_list_prepend(ui_dialog_overrides_entries, entry);
}

void ui_dialog_overrides_create_entries(void)
{
    RinksTournament *tournament = application_get_current_tournament();
    GList *overrides = tournament_get_overrides(tournament);

    guint32 noverrides = g_list_length(overrides);
    GList *tmp;

    guint32 offset;

    /* TODO: fill models */

    if (noverrides > 0) {
        ui_dialog_overrides_table = gtk_table_new(noverrides, OVERRIDES_ENTRY_LENGTH, FALSE);

        for (tmp = overrides, offset = 0; tmp != NULL; tmp = g_list_next(tmp), ++offset) {
            ui_dialog_overrides_add_entry(tournament, ui_dialog_overrides_table, offset,
                    ((RinksOverride *)tmp->data));
        }

        gtk_box_pack_start(GTK_BOX(ui_dialog_overrides), ui_dialog_overrides_table,
                FALSE, FALSE, 0);
        gtk_widget_show_all(ui_dialog_overrides_table);
    }
    else {
        ui_dialog_overrides_table = NULL;
    }

    g_list_free_full(overrides, g_free);
}

void ui_dialog_overrides_clear_entries(void)
{
    g_list_free_full(ui_dialog_overrides_entries, g_free);
    if (ui_dialog_overrides_table != NULL)
        gtk_widget_destroy(ui_dialog_overrides_table);

    ui_dialog_overrides_entries = NULL;
    ui_dialog_overrides_table = NULL;

    ui_dialog_overrides_models_clear();
}

void ui_dialog_overrides_rebuild_entries(void)
{
    ui_dialog_overrides_clear_entries();
    ui_dialog_overrides_create_entries();
}

void ui_dialog_overrides_apply_cb(gpointer data)
{
    g_printf("dialog-overrides: apply\n");
    GList *tmp;

    RinksTournament *tournament = application_get_current_tournament();
    RinksOverride override;

    for (tmp = ui_dialog_overrides_entries; tmp != NULL; tmp = g_list_next(tmp)) {
        override.id = ((struct UiDialogOverridesEntry *)tmp->data)->override_id;
        override.encounter = ui_helper_combo_widget_get_selection(
                ((struct UiDialogOverridesEntry *)tmp->data)->encounter);
        override.team1 = ui_helper_combo_widget_get_selection(
                ((struct UiDialogOverridesEntry *)tmp->data)->teams[0]);
        override.team2 = ui_helper_combo_widget_get_selection(
                ((struct UiDialogOverridesEntry *)tmp->data)->teams[1]);

        tournament_update_override(tournament, &override);
    }

    rounds_update_encounters();
}

void ui_dialog_overrides_activated_cb(gpointer data)
{
    g_printf("dialog-overrides: activated\n");
    ui_dialog_overrides_rebuild_entries();
}

void ui_dialog_overrides_destroy_cb(gpointer data)
{
    g_printf("dialog-overrides: destroy\n");
    ui_dialog_overrides_clear_entries();
}

void ui_dialog_overrides_button_add_override_clicked(GtkButton *button, gpointer data)
{
    RinksTournament *tournament = application_get_current_tournament();

    RinksOverride *override = g_malloc0(sizeof(RinksOverride));
    override->id = tournament_add_override(tournament, override);

    guint rows = 0;

    if (ui_dialog_overrides_table == NULL) {
        ui_dialog_overrides_table = gtk_table_new(1, OVERRIDES_ENTRY_LENGTH, FALSE);

        gtk_box_pack_start(GTK_BOX(ui_dialog_overrides),
                ui_dialog_overrides_table, FALSE, FALSE, 0);
    }
    else {
        gtk_table_get_size(GTK_TABLE(ui_dialog_overrides_table), &rows, NULL);
        gtk_table_resize(GTK_TABLE(ui_dialog_overrides_table), rows + 1, OVERRIDES_ENTRY_LENGTH);
    }

    ui_dialog_overrides_add_entry(tournament, ui_dialog_overrides_table, rows, override);

    gtk_widget_show_all(ui_dialog_overrides_table);
}

GtkWidget *ui_dialog_overrides_open(gpointer data)
{
    static UiDialogCallbacks callbacks = {
        .apply_cb = ui_dialog_overrides_apply_cb,
        .activated_cb = ui_dialog_overrides_activated_cb,
        .destroy_cb = ui_dialog_overrides_destroy_cb
    };

    GtkWidget *button;

    if (!GTK_IS_WIDGET(ui_dialog_overrides)) {
        ui_dialog_overrides = ui_dialog_page_new("Korrekturen", &callbacks);

        button = gtk_button_new_with_label("Neue Korrektur");
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(ui_dialog_overrides_button_add_override_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(ui_dialog_overrides), button, FALSE, FALSE, 3);
    }

    gtk_widget_show_all(ui_dialog_overrides);

    return ui_dialog_overrides;
}
