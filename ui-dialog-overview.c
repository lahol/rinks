/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#include <glib/gprintf.h>

#include "ui-dialog-overview.h"
#include "ui.h"

#include "tournament.h"
#include "teams.h"
#include "rounds.h"
#include "encounters.h"
#include "games.h"
#include "application.h"
#include "output.h"

struct UiDialogOverviewEntry {
    RinksOutputType output_type;
    gint64 output_data;
    GtkWidget *checkbox;
    GtkWidget *output;
};

GtkWidget *ui_dialog_overview = NULL;
GList *ui_dialog_overview_entries = NULL;

void ui_dialog_overview_clear_entries(void)
{
    g_printf("ui-dialog-overview: clear entries\n");
    GList *tmp;
    for (tmp = ui_dialog_overview_entries; tmp != NULL; tmp = g_list_next(tmp)) {
        gtk_widget_destroy(((struct UiDialogOverviewEntry *)tmp->data)->checkbox);
        gtk_widget_destroy(((struct UiDialogOverviewEntry *)tmp->data)->output);
    }
    g_list_free_full(ui_dialog_overview_entries, g_free);

    ui_dialog_overview_entries = NULL;
}

void ui_dialog_overview_create_entry(RinksTournament *tournament, RinksOutputType o_type, gpointer entry_data)
{
    struct UiDialogOverviewEntry *entry = g_malloc0(sizeof(struct UiDialogOverviewEntry));
    entry->output_type = o_type;

    gchar *label = NULL;
    PangoTabArray *tab_array = NULL;

    switch (o_type) {
        case RinksOutputTypeRanking:
            label = g_strdup("Gesamtrangliste");
            tab_array = pango_tab_array_new_with_positions(3, TRUE,
                    PANGO_TAB_LEFT, 300,
                    PANGO_TAB_LEFT, 330,
                    PANGO_TAB_LEFT, 360);
            break;
        case RinksOutputTypeRankingGroup:
            label = g_strdup_printf("Rangliste Gruppe %c",
                    entry_data ? ((RinksTeam *)((GList *)entry_data)->data)->group_id - 1 + 'A' : '-');
            if (entry_data) {
                entry->output_data = ((RinksTeam *)((GList *)entry_data)->data)->group_id;
            }
            tab_array = pango_tab_array_new_with_positions(3, TRUE,
                    PANGO_TAB_LEFT, 300,
                    PANGO_TAB_LEFT, 330,
                    PANGO_TAB_LEFT, 360);
            break;
        case RinksOutputTypeGameEncounter:
            if (entry_data) {
                label = ((RinksGame *)entry_data)->description;
                entry->output_data = ((RinksGame *)entry_data)->id;
            }
            tab_array = pango_tab_array_new_with_positions(2, TRUE,
                    PANGO_TAB_LEFT, 300,
                    PANGO_TAB_LEFT, 320);

            break;
        case RinksOutputTypeRoundEncounter:
            if (entry_data) {
                label = ((RinksRound *)entry_data)->description;
                entry->output_data = ((RinksRound *)entry_data)->id;
            }
            break;
        case RinksOutputTypeNone:
            break;
    }

    entry->checkbox = gtk_check_button_new_with_label(label);
    gtk_box_pack_start(GTK_BOX(ui_dialog_overview), entry->checkbox, FALSE, FALSE, 3);

    entry->output = gtk_text_view_new();
    gtk_widget_set_size_request(entry->output, 200, 200);
    if (tab_array != NULL) {
        gtk_text_view_set_tabs(GTK_TEXT_VIEW(entry->output), tab_array);
    }
    gtk_box_pack_start(GTK_BOX(ui_dialog_overview), entry->output, FALSE, FALSE, 3);

    gchar *content = output_format_plain(tournament, entry->output_type, entry->output_data, entry_data);
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(entry->output));
    if (content != NULL) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), content, -1);
        g_free(content);
    }

    ui_dialog_overview_entries = g_list_prepend(ui_dialog_overview_entries, entry);
}

void ui_dialog_overview_create_entries(void)
{
    g_printf("ui-dialog-overview-create-entries\n");
    RinksTournament *tournament = application_get_current_tournament();
    if (tournament == NULL)
        return;

    GList *games = tournament_get_games(tournament);
    GList *teams = tournament_get_teams(tournament);
  /*  GList *rounds = tournament_get_rounds(tournament);*/
    gint32 ngroups = tournament_get_group_count(tournament);


    GList *tmp;

/*    for (tmp = rounds; tmp != NULL; tmp = g_list_next(tmp)) {
        ui_dialog_overview_create_entry(tournament, RinksOutputTypeRoundEncounter, tmp->data);
    }*/

    for (tmp = games; tmp != NULL; tmp = g_list_next(tmp)) {
        ui_dialog_overview_create_entry(tournament, RinksOutputTypeGameEncounter, tmp->data);
    }

    gint32 i;
    GList *team_group;
    for (i = 1; i <= ngroups; ++i) {
        team_group = teams_filter(teams, RinksTeamFilterTypeGroup, GINT_TO_POINTER(i));
        ui_dialog_overview_create_entry(tournament, RinksOutputTypeRankingGroup, team_group);
        g_list_free(team_group);
    }

    ui_dialog_overview_create_entry(tournament, RinksOutputTypeRanking, teams);

    g_list_free_full(games, (GDestroyNotify)game_free);
    g_list_free_full(teams, (GDestroyNotify)team_free);
/*    g_list_free_full(rounds, (GDestroyNotify)round_free);*/

    gtk_widget_show_all(ui_dialog_overview);
}

void ui_dialog_overview_rebuild_entries(void)
{
    ui_dialog_overview_clear_entries();
    ui_dialog_overview_create_entries();
}

void ui_dialog_overview_activate_cb(gpointer data)
{
    g_printf("ui-dialog-overview: activate\n");
    tournament_update_standings(application_get_current_tournament());
    ui_dialog_overview_rebuild_entries();
}

void ui_dialog_overview_destroy_cb(gpointer data)
{
    ui_dialog_overview_clear_entries();
}

void ui_dialog_overview_button_print_clicked(GtkButton *button, gpointer data)
{
    g_printf("ui-dialog-overview: output button clicked\n");

    gchar *filename = ui_get_filename(GTK_FILE_CHOOSER_ACTION_SAVE, RINKS_FILE_PDF);
    if (filename == NULL)
        return;

    output_init();

    GList *tmp;

    for (tmp = ui_dialog_overview_entries; tmp != NULL; tmp = g_list_next(tmp)) {
        if (gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(((struct UiDialogOverviewEntry *)tmp->data)->checkbox))) {
            output_add(((struct UiDialogOverviewEntry *)tmp->data)->output_type,
                    ((struct UiDialogOverviewEntry *)tmp->data)->output_data);
        }
    }

    output_print(application_get_current_tournament(), filename);
}

GtkWidget *ui_dialog_overview_open(gpointer data)
{
    static UiDialogCallbacks callbacks = {
        .activated_cb = ui_dialog_overview_activate_cb,
        .destroy_cb = ui_dialog_overview_destroy_cb
    };

    GtkWidget *button;

    if (!GTK_IS_WIDGET(ui_dialog_overview)) {
        ui_dialog_overview = ui_dialog_page_new("Übersicht", &callbacks);

        button = gtk_button_new_with_label("Ausgewählte ausgeben");

        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(ui_dialog_overview_button_print_clicked), NULL);

        gtk_box_pack_start(GTK_BOX(ui_dialog_overview), button, FALSE, FALSE, 3);
    }

    gtk_widget_show_all(ui_dialog_overview);

    return ui_dialog_overview;
}
