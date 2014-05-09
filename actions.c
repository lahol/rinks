#include "actions.h"
#include "db.h"
#include "tournament.h"
#include <glib/gprintf.h>
#include <memory.h>
#include "ui.h"

RinksActionCallbacks action_callbacks;

RinksTournament *action_new_tournament(void)
{
    gchar *filename = ui_get_filename(GTK_FILE_CHOOSER_ACTION_SAVE, RINKS_FILE_TOURNAMENT);
    if (filename == NULL)
        return NULL;

    RinksTournament *tournament = tournament_open(filename, TRUE);
    g_free(filename);

    if (action_callbacks.handle_tournament_changed)
        action_callbacks.handle_tournament_changed(tournament);

    return tournament;
}

RinksTournament *action_open_tournament(void)
{
    gchar *filename = ui_get_filename(GTK_FILE_CHOOSER_ACTION_OPEN, RINKS_FILE_TOURNAMENT);
    if (filename == NULL)
        return NULL;

    RinksTournament *tournament = tournament_open(filename, FALSE);
    g_free(filename);

    if (action_callbacks.handle_tournament_changed)
        action_callbacks.handle_tournament_changed(tournament);

    return tournament;
}

void action_set_callbacks(RinksActionCallbacks *callbacks)
{
    if (callbacks)
        memcpy(&action_callbacks, callbacks, sizeof(RinksActionCallbacks));
}
