#include <glib/gprintf.h>

#include "ui-dialog-games.h"
#include "ui.h"

#include "tournament.h"
#include "teams.h"
#include "rounds.h"
#include "application.h"

GtkWidget *ui_dialog_games = NULL;

void ui_dialog_games_apply_cb(gpointer data)
{
    g_printf("ui-dialog-games: destroy\n");
}

void ui_dialog_games_destroy_cb(gpointer data)
{
    g_printf("ui-dialog-games: destroy\n");
}

void ui_dialog_games_activated_cb(gpointer data)
{
    g_printf("ui-dialog-games: activated with data: %p\n", data);
}

void ui_dialog_games_deactivated_cb(gpointer data)
{
    g_printf("ui-dialog-games: deactivated\n");
}

void ui_dialog_games_data_changed_cb(gpointer data)
{
    g_printf("ui-dialog-games: data changed\n");
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

    if (!GTK_IS_WIDGET(ui_dialog_games)) {
        ui_dialog_games = ui_dialog_page_new("Spiele", &callbacks);
    }

    gtk_widget_show_all(ui_dialog_games);

    return ui_dialog_games;
}
