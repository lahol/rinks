#include "tournament.h"
#include "data.h"
#include "db.h"
#include <stdlib.h>

struct _RinksTournament {
    gpointer db_handle;
    gchar *filename;
    gchar *description;
    gint rink_count;
    GList *rounds; /* [element-type: RinksRound] */
    GList *teams;  /* [element-type: RinksTeam] */
};

void tournament_read_data(RinksTournament *tournament)
{
    g_return_if_fail(tournament != NULL);
    
    tournament_set_description(tournament, db_get_property(tournament->db_handle, "tournament.description"));

    const gchar *val;
    val = db_get_property(tournament->db_handle, "tournament.rink-count");
    if (val != NULL && val[0] != '\0')
        tournament->rink_count = atoi(val);
    else
        tournament->rink_count = 1;
}

RinksTournament *tournament_open(gchar *filename, gboolean clear)
{
    g_return_val_if_fail(filename != NULL, NULL);
    g_return_val_if_fail(filename[0] != '\0', NULL);

    gpointer db_handle;
    if ((db_handle = db_open_database(filename, clear)) == NULL)
        return NULL;

    RinksTournament *tournament = g_malloc0(sizeof(RinksTournament));

    tournament->db_handle = db_handle;
    tournament->filename = g_strdup(filename);

    if (!clear)
        tournament_read_data(tournament);

    return tournament;
}

void tournament_close(RinksTournament *tournament)
{
    g_return_if_fail(tournament != NULL);

    g_free(tournament->filename);
    g_free(tournament->description);
    /* TODO: free rounds, teams */
    db_close_database(tournament->db_handle);

    g_free(tournament);
}

void tournament_set_description(RinksTournament *tournament, const gchar *description)
{
    g_return_if_fail(tournament != NULL);

    if (tournament->description != NULL)
        g_free(tournament->description);

    tournament->description = g_strdup(description);
}

const gchar *tournament_get_description(RinksTournament *tournament)
{
    g_return_val_if_fail(tournament != NULL, NULL);

    return tournament->description;
}
