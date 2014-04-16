#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include "ui.h"
#include "actions.h"
#include "db.h"
#include "tournament.h"

void menu_callback(gchar *action);

RinksTournament *current_tournament = NULL;

void tournament_changed_cb(RinksTournament *tournament);

void init(void)
{
    ui_create_main_window();
    ui_set_action_callback(menu_callback);

    RinksActionCallbacks callbacks = {
        .handle_tournament_changed = tournament_changed_cb
    };

    action_set_callbacks(&callbacks);
}

void cleanup(void)
{
    if (current_tournament)
        tournament_close(current_tournament);
}

void tournament_changed_cb(RinksTournament *tournament)
{
    g_printf("Tournament changed\n");
    if (current_tournament != NULL)
        tournament_close(current_tournament);

    current_tournament = tournament;

    ui_select_view(VIEW_SETTINGS, current_tournament);
}

void menu_callback(gchar *action)
{
    if (g_strcmp0(action, "actions.quit") == 0) {
        gtk_main_quit();
    }
    else if (g_strcmp0(action, "actions.new") == 0) {
        g_printf("New Tournament\n");
        action_new_tournament();
    }
    else if (g_strcmp0(action, "actions.open") == 0) {
        g_printf("Open Tournament\n");
        action_open_tournament();
    }
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    init();

    gtk_main();

    cleanup();

    return 0;
}
