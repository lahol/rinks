#include "tournament.h"
#include "data.h"
#include "db.h"
#include <stdlib.h>

struct _RinksTournament {
    gpointer db_handle;
    gchar *filename;
    gchar *description;
    gint rink_count;
    gint group_count;
    GList *rounds; /* [element-type: RinksRound] */
    GList *teams;  /* [element-type: RinksTeam] */
};

void tournament_read_data(RinksTournament *tournament)
{
    g_return_if_fail(tournament != NULL);
    
    gchar *val;
    val = db_get_property(tournament->db_handle, "tournament.description"); 
    tournament_set_description(tournament, val);
    g_free(val);

    val = db_get_property(tournament->db_handle, "tournament.rink-count");
    if (val != NULL && val[0] != '\0')
        tournament->rink_count = atoi(val);
    else
        tournament->rink_count = 0;
    g_free(val);

    val = db_get_property(tournament->db_handle, "tournament.group-count");
    if (val != NULL && val[0] != '\0')
        tournament->group_count = atoi(val);
    else
        tournament->group_count = 0;
    g_free(val);
}

void tournament_write_data(RinksTournament *tournament)
{
    g_return_if_fail(tournament != NULL);

    gchar *val = NULL;

    db_set_property(tournament->db_handle, "tournament.description",
            tournament->description);

    val = g_strdup_printf("%d", tournament->rink_count);
    db_set_property(tournament->db_handle, "tournament.rink-count", val);
    g_free(val);

    val = g_strdup_printf("%d", tournament->group_count);
    db_set_property(tournament->db_handle, "tournament.group-count", val);
    g_free(val);
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

void tournament_set_rink_count(RinksTournament *tournament, gint rink_count)
{
    g_return_if_fail(tournament != NULL);

    tournament->rink_count = rink_count;
}

gint tournament_get_rink_count(RinksTournament *tournament)
{
    g_return_val_if_fail(tournament != NULL, 0);

    return tournament->rink_count;
}

void tournament_set_group_count(RinksTournament *tournament, gint group_count)
{
    g_return_if_fail(tournament != NULL);

    tournament->group_count = group_count;
}

gint tournament_get_group_count(RinksTournament *tournament)
{
    g_return_val_if_fail(tournament != NULL, 0);

    return tournament->group_count;
}

void tournament_set_property(RinksTournament *tournament, const gchar *key, const gchar *value)
{
    g_return_if_fail(tournament != NULL);

    if (g_strcmp0(key, "tournament.description") == 0) {
        tournament_set_description(tournament, value);
    }
    else if (g_strcmp0(key, "tournament.rink-count") == 0) {
        tournament_set_rink_count(tournament, value ? atoi(value) : 0);
    }
    else if (g_strcmp0(key, "tournament.group-count") == 0) {
        tournament_set_group_count(tournament, value ? atoi(value) : 0);
    }
}

gchar *tournament_get_property(RinksTournament *tournament, const gchar *key)
{
    g_return_val_if_fail(tournament != NULL, NULL);

    if (g_strcmp0(key, "tournament.description") == 0) {
        return g_strdup(tournament_get_description(tournament));
    }
    else if (g_strcmp0(key, "tournament.rink-count") == 0) {
        return g_strdup_printf("%d", tournament_get_rink_count(tournament));
    }
    else if (g_strcmp0(key, "tournament.group-count") == 0) {
        return g_strdup_printf("%d", tournament_get_group_count(tournament));
    }

    return NULL;
}

GList *tournament_get_teams(RinksTournament *tournament)
{
    g_return_val_if_fail(tournament != NULL, NULL);

    return db_get_teams(tournament->db_handle);
}

gint64 tournament_add_team(RinksTournament *tournament, RinksTeam *team)
{
    g_return_val_if_fail(tournament != NULL, -1);

    return db_add_team(tournament->db_handle, team);
}

void tournament_update_team(RinksTournament *tournament, RinksTeam *team)
{
    g_return_if_fail(tournament != NULL);

    db_update_team(tournament->db_handle, team);
}

