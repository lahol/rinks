/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#include <glib/gprintf.h>

#include "ui-dialog-teams.h"
#include "ui.h"

#include "tournament.h"
#include "teams.h"
#include "application.h"

GtkWidget *ui_dialog_teams = NULL;
GtkWidget *ui_dialog_teams_table = NULL;

struct UiDialogTeamsEntry {
    gint64 team_id;
    GtkWidget *team_name;
    GtkWidget *team_skip;
    GtkWidget *team_group;
};

GList *ui_dialog_teams_entries = NULL; /* [element-type: struct UiDialogTeamsEntry] */

void ui_dialog_teams_create_entries(void);
void ui_dialog_teams_add_entry(GtkWidget *table, gint offset, RinksTeam *team, gint ngroups);
void ui_dialog_teams_clear_entries(void);
void ui_dialog_teams_rebuild_entries(void);

void ui_dialog_teams_apply_cb(gpointer data)
{
    g_printf("dialog-teams: apply\n");

    GList *tmp;
    RinksTeam team;
    struct UiDialogTeamsEntry *entry;
    RinksTournament *tournament = application_get_current_tournament();

    for (tmp = ui_dialog_teams_entries; tmp != NULL; tmp = g_list_next(tmp)) {
        entry = (struct UiDialogTeamsEntry *)tmp->data;
        if (entry == NULL)
            continue;
        team.id = entry->team_id;
        team.name = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry->team_name)));
        team.skip = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry->team_skip)));
        team.group_id = gtk_combo_box_get_active(GTK_COMBO_BOX(entry->team_group)) + 1;
        if (team.group_id == 0)
            team.group_id = 1;

        team.valid_keys = RinksTeamKeyName | RinksTeamKeySkip | RinksTeamKeyGroupId;

        tournament_update_team(tournament, &team);
        g_free(team.name);
        g_free(team.skip);
    }
}

void ui_dialog_teams_update_cb(gpointer data)
{
    g_printf("dialog-teams: update\n");
}

void ui_dialog_teams_destroy_cb(gpointer data)
{
    g_printf("dialog-teams: destroy\n");
    ui_dialog_teams_clear_entries();
}

void ui_dialog_teams_activated_cb(gpointer data)
{
    g_printf("dialog-teams: activated\n");
}

void ui_dialog_teams_deactivated_cb(gpointer data)
{
    g_printf("dialog-teams: deactivated\n");
}

void ui_dialog_teams_data_changed_cb(gpointer data)
{
    g_printf("dialog-teams: data changed\n");
    ui_dialog_teams_rebuild_entries();
}

void ui_dialog_teams_button_add_team_clicked(GtkButton *button, gpointer data)
{
    RinksTournament *tournament = application_get_current_tournament();
    gint ngroups = tournament_get_group_count(tournament);
    RinksTeam *team = g_malloc0(sizeof(RinksTeam));

    team->name = g_strdup("Teamname");
    team->skip = g_strdup("Skip");
    team->group_id = 1;
    team->id = -1;

    team->id = tournament_add_team(tournament, team);

    guint rows = 0;

    if (ui_dialog_teams_table == NULL) {
        ui_dialog_teams_table = gtk_table_new(1, 4, FALSE);

        gtk_box_pack_start(GTK_BOX(ui_dialog_teams), ui_dialog_teams_table, FALSE, FALSE, 0);
    }
    else {
        gtk_table_get_size(GTK_TABLE(ui_dialog_teams_table), &rows, NULL);
        gtk_table_resize(GTK_TABLE(ui_dialog_teams_table), rows + 1, 4);
    }
    ui_dialog_teams_add_entry(ui_dialog_teams_table, rows, team, ngroups);

    gtk_widget_show_all(ui_dialog_teams_table);
}

void ui_dialog_teams_create_entries(void)
{
    RinksTournament *tournament = application_get_current_tournament();
    gint ngroups = tournament_get_group_count(tournament);
    GList *teams = tournament_get_teams(tournament);
    GList *tmp;

    gint nteams = g_list_length(teams);
    g_printf("ui-dialog-teams: create entries: nteams: %d\n", nteams);
    gint offset;

    if (nteams > 0) {
        ui_dialog_teams_table = gtk_table_new(nteams, 4, FALSE);

        for (tmp = teams, offset = 0; tmp != NULL; tmp = g_list_next(tmp), ++offset) {
            ui_dialog_teams_add_entry(ui_dialog_teams_table, offset, (RinksTeam *)tmp->data, ngroups);
        }

        gtk_box_pack_start(GTK_BOX(ui_dialog_teams), ui_dialog_teams_table, FALSE, FALSE, 0);

        gtk_widget_show_all(ui_dialog_teams_table);
    }
    else {
        ui_dialog_teams_table = NULL;
    }

    g_list_free_full(teams, (GDestroyNotify)team_free);
}

void ui_dialog_teams_init_group_widget(GtkWidget *combobox, gint ngroups, gint selection)
{
    g_return_if_fail(combobox != NULL);

    gint i;
    gchar *text;
    for (i = 0; i < ngroups; ++i) {
        text = g_strdup_printf("Gruppe %c", (gchar)(i + 'A'));
        gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(combobox), i, text);
        g_free(text);
    }

    if (selection > 0 && selection <= ngroups) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), selection - 1);
    }
}

void ui_dialog_teams_add_entry(GtkWidget *table, gint offset, RinksTeam *team, gint ngroups)
{
    g_return_if_fail(team != NULL);
    g_return_if_fail(GTK_IS_WIDGET(table));

    struct UiDialogTeamsEntry *entry = g_malloc0(sizeof(struct UiDialogTeamsEntry));
    entry->team_id = team->id;

    gchar *text = g_strdup_printf("Team %" G_GINT64_FORMAT ":", team->id);
    GtkWidget *label = gtk_label_new(text);
    g_free(text);

    gtk_table_attach(GTK_TABLE(table), label, 0, 1, offset, offset + 1, 0, 0, 2, 2);

    entry->team_name = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry->team_name), team->name ? team->name : "");
    gtk_table_attach(GTK_TABLE(table), entry->team_name, 1, 2, offset, offset + 1, 0, 0, 2, 2);

    entry->team_skip = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry->team_skip), team->skip ? team->skip : "");
    gtk_table_attach(GTK_TABLE(table), entry->team_skip, 2, 3, offset, offset + 1, 0, 0, 2, 2);

    entry->team_group = gtk_combo_box_text_new();
    ui_dialog_teams_init_group_widget(entry->team_group, ngroups, team->group_id);
    gtk_table_attach(GTK_TABLE(table), entry->team_group, 3, 4, offset, offset + 1, 0, 0, 2, 2);

    ui_dialog_teams_entries = g_list_append(ui_dialog_teams_entries, entry);
}

void ui_dialog_teams_clear_entries(void)
{
/*    GList *tmp = NULL;
    GtkWidget *box = NULL;

    for (tmp = ui_dialog_teams_entries; tmp != NULL; tmp = g_list_next(tmp)) {
        gtk_widget_destroy(((struct UiDialogTeamsEntry *)tmp->data)->hbox);
    }*/

    if (ui_dialog_teams_table != NULL)
        gtk_widget_destroy(ui_dialog_teams_table);

    ui_dialog_teams_table = NULL;

    g_list_free_full(ui_dialog_teams_entries, g_free);
    ui_dialog_teams_entries = NULL;
}

void ui_dialog_teams_rebuild_entries(void)
{
    ui_dialog_teams_clear_entries();
    ui_dialog_teams_create_entries();
}

GtkWidget *ui_dialog_teams_open(gpointer data)
{
    static UiDialogCallbacks callbacks = {
        .apply_cb = ui_dialog_teams_apply_cb,
        .update_cb = ui_dialog_teams_update_cb,
        .destroy_cb = ui_dialog_teams_destroy_cb,
        .activated_cb = ui_dialog_teams_activated_cb,
        .deactivated_cb = ui_dialog_teams_deactivated_cb,
        .data_changed_cb = ui_dialog_teams_data_changed_cb
    };

    GtkWidget *button;

    if (!GTK_IS_WIDGET(ui_dialog_teams)) {
        ui_dialog_teams = ui_dialog_page_new("Teams", &callbacks);

        button = gtk_button_new_with_label("Neues Team");

        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(ui_dialog_teams_button_add_team_clicked), NULL);

        gtk_box_pack_start(GTK_BOX(ui_dialog_teams), button, FALSE, FALSE, 3);
    }

    gtk_widget_show_all(ui_dialog_teams);

    return ui_dialog_teams;
}
