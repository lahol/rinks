#include <glib/gprintf.h>

#include "ui-dialog-teams.h"
#include "ui.h"

#include "data.h"

#include "tournament.h"
#include "application.h"

GtkWidget *ui_dialog_teams = NULL;
GtkWidget *ui_dialog_teams_table = NULL;

struct UiDialogTeamsEntry {
    gint team_id;
    GtkWidget *team_name;
    GtkWidget *team_skip;
    GtkWidget *team_group;
};

GList *ui_dialog_teams_entries = NULL; /* [element-type: struct UiDialogTeamsEntry] */

void ui_dialog_teams_create_entries(void);
void ui_dialog_teams_add_entry(RinksTeam *team);
void ui_dialog_teams_clear_entries(void);
void ui_dialog_rebuild_entries(void);

void ui_dialog_teams_apply_cb(gpointer data)
{
    g_printf("dialog-teams: apply\n");
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
    ui_dialog_rebuild_entries();
}

void ui_dialog_teams_create_entries(void)
{
    RinksTournament *tournament = application_get_current_tournament();
    GList *teams = tournament_get_teams(tournament);
    GList *tmp;

    gint nteams = g_list_length(teams);
    gint offset;

    if (nteams > 0) {
        ui_dialog_teams_table = gtk_table_new(nteams, 4, FALSE);

        GtkWidget *entry;

        for (tmp = teams, offset = 0; tmp != NULL; tmp = g_list_next(tmp), ++offset) {
            ui_dialog_teams_add_entry(ui_dialog_teams_table, offset, (RinksTeam *)tmp->data);
        }

        gtk_box_pack_start(GTK_BOX(ui_dialog_teams), ui_dialog_teams_table, FALSE, FALSE, 0);
    }
    else {
        ui_dialog_teams_table = NULL;
    }

    g_list_free_full(teams, team_free);
    return NULL;
}

void ui_dialog_teams_add_entry(GtkWidget *table, gint offset, RinksTeam *team)
{
    g_return_val_if_fail(team != NULL, NULL);

    struct UiDialogTeamsEntry *entry = g_malloc0(sizeof(struct UiDialogTeamsEntry));

    /* TODO: create label and entries, pack them */

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

    g_list_free_full(ui_dialog_teams_entries, g_free);
}

void ui_dialog_rebuild_entries(void)
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
    if (!GTK_IS_WIDGET(ui_dialog_teams)) {
        ui_dialog_teams = ui_dialog_page_new("Teams", &callbacks);
    }

    gtk_widget_show_all(ui_dialog_teams);

    return ui_dialog_teams;
}
