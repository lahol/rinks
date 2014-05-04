#include <glib/gprintf.h>

#include "ui-dialog-games.h"
#include "ui.h"

#include "tournament.h"
#include "teams.h"
#include "rounds.h"
#include "encounters.h"
#include "games.h"
#include "application.h"

struct UiDialogGamesEntry {
    gint64 game_id;
    GtkWidget *frame;
    GtkWidget *description;
    GtkWidget **encounters;
    gint32 nencounters;
};

GtkWidget *ui_dialog_games = NULL;
GList *ui_dialog_games_entries = NULL;
GtkTreeModel *ui_dialog_games_encounter_model = NULL;
gboolean ui_dialog_games_encounter_model_built = FALSE;

void ui_dialog_games_clear_tree_model(void)
{
    g_printf("ui-dialog-games: clear tree model\n");
    if (ui_dialog_games_encounter_model != NULL) {
        gtk_list_store_clear(GTK_LIST_STORE(ui_dialog_games_encounter_model));
        ui_dialog_games_encounter_model_built = FALSE;
    }
}

void ui_dialog_games_build_tree_model(GList *encounters)
{
    g_printf("ui-dialog-games: build tree model\n");
    if (ui_dialog_games_encounter_model == NULL) {
        ui_dialog_games_encounter_model = GTK_TREE_MODEL(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT64));
    }
    GList *tmp;
    GtkTreeIter iter;
    gchar *text;

    gtk_list_store_append(GTK_LIST_STORE(ui_dialog_games_encounter_model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(ui_dialog_games_encounter_model), &iter,
            0, "-- Begegnung auswählen --",
            1, 0,
            -1);

    for (tmp = encounters; tmp != NULL; tmp = g_list_next(tmp)) {
        text = encounters_translate((RinksEncounter *)tmp->data);
        gtk_list_store_append(GTK_LIST_STORE(ui_dialog_games_encounter_model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(ui_dialog_games_encounter_model), &iter,
                0, text,
                1, ((RinksEncounter *)tmp->data)->id,
                -1);
        g_free(text);
    }

    ui_dialog_games_encounter_model_built = TRUE;
}

void ui_dialog_games_encounter_widget_set_selection(GtkWidget *widget, gint64 encounter)
{
    g_printf("ui-dialog-games: widget set selection\n");
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ui_dialog_games_encounter_model),
            &iter);

    gint64 id;
    while (valid) {
        gtk_tree_model_get(GTK_TREE_MODEL(ui_dialog_games_encounter_model), &iter, 1, &id, -1);

        if (id == encounter) {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget), &iter);
            break;
        }

        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(ui_dialog_games_encounter_model), &iter);
    }
}

gint64 ui_dialog_games_encounter_widget_get_selection(GtkWidget *widget)
{
    g_printf("ui-dialog-games: widget get selection\n");
    GtkTreeIter iter;

    if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
        return -1;

    gint64 id;
    gtk_tree_model_get(GTK_TREE_MODEL(ui_dialog_games_encounter_model), &iter, 1, &id, -1);

    return id;
}

void ui_dialog_games_create_entry(RinksTournament *tournament, RinksGame *game, gint32 nrinks)
{
    g_printf("ui-dialog-games: create entry\n");
    g_return_if_fail(tournament != NULL);
    g_return_if_fail(game != NULL);

    struct UiDialogGamesEntry *entry = g_malloc0(sizeof(struct UiDialogGamesEntry));
    entry->encounters = g_malloc0(sizeof(GtkWidget *) * nrinks);
    entry->nencounters = nrinks;
    entry->game_id = game->id;

    entry->frame = gtk_frame_new(NULL);

    GtkWidget *table = gtk_table_new(nrinks + 1, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(entry->frame), table);

    GtkWidget *label;
    gint32 i;
    gchar *text;

    label = gtk_label_new("Beschreibung:");
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, 0, 0, 2, 2);

    entry->description = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry->description), game->description ? game->description : "");
    gtk_table_attach(GTK_TABLE(table), entry->description, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 2, 2);

    GList *encounters = tournament_get_encounters(tournament, 0, game->id);
    GList *all_encounters = NULL;
    GList *tmp;
    RinksEncounter *encounter;

    if (ui_dialog_games_encounter_model == NULL || !ui_dialog_games_encounter_model_built) {
        all_encounters = encounters_sort(tournament_get_encounters(tournament, 0, 0), RinksEncounterSortLogical);
        ui_dialog_games_build_tree_model(all_encounters);
        g_list_free_full(all_encounters, (GDestroyNotify)encounter_free);
    }

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    for (i = 0; i < nrinks; ++i) {
        text = g_strdup_printf("Rink %c:", (gchar)(i + 'A'));
        label = gtk_label_new(text);
        g_free(text);

        gtk_table_attach(GTK_TABLE(table), label, 0, 1, i + 1, i + 2, 0, 0, 2, 2);

        entry->encounters[i] = gtk_combo_box_new_with_model(GTK_TREE_MODEL(ui_dialog_games_encounter_model));
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(entry->encounters[i]), renderer, TRUE);
        gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(entry->encounters[i]), renderer, "text", 0);

        gtk_table_attach(GTK_TABLE(table), entry->encounters[i], 1, 2, i + 1, i + 2, GTK_EXPAND | GTK_FILL, 0, 2, 2);
    }

    for (tmp = encounters; tmp != NULL; tmp = g_list_next(tmp)) {
        encounter = (RinksEncounter *)tmp->data;
        if (encounter == NULL)
            continue;

        if (encounter->rink <= 0 || encounter->rink > nrinks) {
            g_printf("invalid rink of encounter: %d\n", encounter->rink);
            continue;
        }

        ui_dialog_games_encounter_widget_set_selection(entry->encounters[encounter->rink - 1], encounter->id);
    }

    g_list_free_full(encounters, (GDestroyNotify)encounter_free);

    gtk_box_pack_start(GTK_BOX(ui_dialog_games), entry->frame, FALSE, FALSE, 3);
    
    gtk_widget_show_all(entry->frame);
    ui_dialog_games_entries = g_list_prepend(ui_dialog_games_entries, entry);
}

void ui_dialog_games_create_entries(void)
{
    g_printf("ui-dialog-games: create entries\n");
    RinksTournament *tournament = application_get_current_tournament();
    if (tournament == NULL)
        return;

    GList *games = tournament_get_games(tournament);
    GList *tmp;

    gint32 nrinks = tournament_get_rink_count(tournament);

    for (tmp = games; tmp != NULL; tmp = g_list_next(tmp)) {
        ui_dialog_games_create_entry(tournament, (RinksGame *)tmp->data, nrinks);
    }

    g_list_free_full(games, (GDestroyNotify)game_free);
}

void ui_dialog_games_clear_entries(void)
{
    g_printf("ui-dialog-games: clear entries\n");
    GList *tmp; 
    for (tmp = ui_dialog_games_entries; tmp != NULL; tmp = g_list_next(tmp)) {
        gtk_widget_destroy(((struct UiDialogGamesEntry *)tmp->data)->frame);
        g_free(((struct UiDialogGamesEntry *)tmp->data)->encounters);
    }

    ui_dialog_games_clear_tree_model();

    g_list_free_full(ui_dialog_games_entries, g_free);
    ui_dialog_games_entries = NULL;
}

void ui_dialog_games_rebuild_entries(void)
{
    g_printf("ui-dialog-games: rebuild entries\n");
    ui_dialog_games_clear_entries();
    ui_dialog_games_create_entries();
}

void ui_dialog_games_apply_cb(gpointer data)
{
    g_printf("ui-dialog-games: apply\n");
    GList *tmp, *enc, *encounters;
    gint32 i;
    gint64 enc_id;
    const gchar *text;
    RinksGame *game;
    RinksEncounter *encounter;

    RinksTournament *tournament = application_get_current_tournament();
    g_printf("tournament: %p\n", tournament);
    if (tournament == NULL)
        return;

    for (tmp = ui_dialog_games_entries; tmp != NULL; tmp = g_list_next(tmp)) {
        g_printf("tmp->data: %p\n", tmp->data);
        if (tmp->data == NULL)
            continue;
        game = tournament_get_game(tournament, ((struct UiDialogGamesEntry *)tmp->data)->game_id);
        g_printf("game: %p\n", game);
        if (game == NULL)
            continue;

        text = gtk_entry_get_text(GTK_ENTRY(((struct UiDialogGamesEntry *)tmp->data)->description));
        g_free(game->description);

        game->description = g_strdup(text);
        g_printf("set description: %s\n", game->description);

        tournament_update_game(tournament, game);
        game_free(game);

        encounters = tournament_get_encounters(tournament, 0, ((struct UiDialogGamesEntry *)tmp->data)->game_id);
        for (enc = encounters; enc != NULL; enc = g_list_next(enc)) {
            tournament_encounter_set_game(tournament, ((RinksEncounter *)enc->data)->id, 0);
        }
        g_list_free_full(encounters, (GDestroyNotify)encounter_free);

        for (i = 0; i < ((struct UiDialogGamesEntry *)tmp->data)->nencounters; ++i) {
            g_printf("i: %d\n", i);
            enc_id = ui_dialog_games_encounter_widget_get_selection(((struct UiDialogGamesEntry *)tmp->data)->encounters[i]);
            if (enc_id > 0 &&
                    (encounter = tournament_get_encounter(tournament, enc_id)) != NULL) {
                encounter->game = ((struct UiDialogGamesEntry *)tmp->data)->game_id;
                encounter->rink = i + 1;
                tournament_update_encounter(tournament, encounter);
            }
        }
    }
}

void ui_dialog_games_destroy_cb(gpointer data)
{
    g_printf("ui-dialog-games: destroy\n");
    ui_dialog_games_clear_entries();
}

void ui_dialog_games_activated_cb(gpointer data)
{
    g_printf("ui-dialog-games: activated with data: %p\n", data);
    ui_dialog_games_rebuild_entries();
}

void ui_dialog_games_deactivated_cb(gpointer data)
{
    g_printf("ui-dialog-games: deactivated\n");
}

void ui_dialog_games_data_changed_cb(gpointer data)
{
    g_printf("ui-dialog-games: data changed\n");
}

void ui_dialog_games_button_add_game_clicked(GtkButton *button, gpointer data)
{
    RinksTournament *tournament = application_get_current_tournament();
    if (tournament == NULL)
        return;

    RinksGame game;
    gint32 nrinks = tournament_get_rink_count(tournament);

    game.closed = 0;
    game.sequence = 0;

    game.description = g_strdup_printf("Neues Spiel");
    game.id = tournament_add_game(tournament, &game);

    ui_dialog_games_create_entry(tournament, &game, nrinks);

    g_free(game.description);
}

GtkWidget *ui_dialog_games_open(gpointer data)
{
    static UiDialogCallbacks callbacks = {
        .apply_cb = ui_dialog_games_apply_cb,
        .destroy_cb = ui_dialog_games_destroy_cb,
        .activated_cb = ui_dialog_games_activated_cb,
        .deactivated_cb = ui_dialog_games_deactivated_cb,
        .data_changed_cb = ui_dialog_games_data_changed_cb
    };

    GtkWidget *button;

    if (!GTK_IS_WIDGET(ui_dialog_games)) {
        ui_dialog_games = ui_dialog_page_new("Spiele", &callbacks);

        button = gtk_button_new_with_label("Neues Spiel");

        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(ui_dialog_games_button_add_game_clicked), NULL);

        gtk_box_pack_start(GTK_BOX(ui_dialog_games), button, FALSE, FALSE, 3);
    }

    gtk_widget_show_all(ui_dialog_games);

    return ui_dialog_games;
}
