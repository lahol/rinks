#include "output.h"
#include <glib/gprintf.h>

typedef struct {
    RinksOutputType type;
    gint64 data;
} RinksOutputData;

GList *output_data = NULL;

void output_init(void)
{
}

void output_clear(void)
{
}

void output_add(RinksOutputType type, gint64 data)
{
}

gboolean output_print(RinksTournament *tournament, const gchar *filename)
{
    return TRUE;
}

gchar **output_format_game_encounter(RinksTournament *tournament, RinksEncounter *encounter)
{
    g_return_val_if_fail(tournament != NULL, NULL);
    g_return_val_if_fail(encounter != NULL, NULL);

    RinksTeam *teams[2];
    teams[0] = tournament_get_team(tournament, encounter->real_team1);
    teams[1] = tournament_get_team(tournament, encounter->real_team2);

    gchar **output = g_malloc0(sizeof(gchar *) * 4);

    output[0] = g_strdup_printf("Rink %c", encounter->rink > 0 ? (gchar)(encounter->rink - 1 + 'A') : '-');
    output[1] = teams[0] != NULL ? g_strdup(teams[0]->name) : g_strdup("???");
    output[2] = teams[1] != NULL ? g_strdup(teams[1]->name) : g_strdup("???");

    team_free(teams[0]);
    team_free(teams[1]);

    return output;
}

/* return nteams * 5 array: pos, name, pts, ends, stones */
gchar **output_format_standings_raw(RinksTournament *tournament, RinksOutputType type, gint64 data, GList *teams_pf)
{
    GList *teams_sorted = NULL;
    GList *teams_filtered = NULL;
    GList *teams = NULL;

    if (teams_pf != NULL) {
        teams_filtered = g_list_copy(teams_pf);
    }
    else {
        teams = tournament_get_teams(tournament);
        if (type == RinksOutputTypeRankingGroup) {
            teams_filtered = teams_filter(teams, RinksTeamFilterTypeGroup, GINT_TO_POINTER((gint32)data));
        }
        else {
            teams_filtered = g_list_copy(teams);
        }
    }

    teams_sorted = teams_sort(teams_filtered, RinksTeamSortAll);

    RinksTeam *pred = NULL;
    GList *tmp;

    guint32 nteams = g_list_length(teams_sorted);
    guint32 i;

    gchar **output = g_malloc0(sizeof(gchar *) * (5 * nteams + 1));

    for (tmp = teams_sorted, i = 0; tmp != NULL && i < nteams; tmp = g_list_next(tmp), ++i) {
        output[5 * i + 1] = g_strdup(((RinksTeam *)tmp->data)->name);
        output[5 * i + 2] = g_strdup_printf("%d", ((RinksTeam *)tmp->data)->points);
        output[5 * i + 3] = g_strdup_printf("%d", ((RinksTeam *)tmp->data)->ends);
        output[5 * i + 4] = g_strdup_printf("%d", ((RinksTeam *)tmp->data)->stones);

        if (pred && teams_compare_standings(pred, ((RinksTeam *)tmp->data)) == 0) {
            output[5 * i] = g_strdup(output[5 * (i - 1)]);
        }
        else {
            output[5 * i] = g_strdup_printf("%" G_GUINT32_FORMAT , (i + 1));
        }

        pred = (RinksTeam *)tmp->data;
    }

    g_list_free(teams_filtered);
    g_list_free_full(teams, (GDestroyNotify)team_free);

    return output;
}

gchar *output_format_game_plain(RinksTournament *tournament, gint64 data, RinksGame *game_pf)
{
    RinksGame *game = game_pf != NULL ? game_pf : tournament_get_game(tournament, data);
    if (game == NULL)
        return NULL;
    GList *encounters = tournament_get_encounters(tournament, 0, data);
    if (encounters == NULL)
        return NULL;

    GString *str = g_string_new("");
    GList *tmp;
    gchar **line = NULL;
    for (tmp = encounters; tmp != NULL; tmp = g_list_next(tmp)) {
        line = output_format_game_encounter(tournament, ((RinksEncounter *)tmp->data));
        if (line != NULL) {
            g_string_append_printf(str, "%s: %s\t–\t%s\n", line[0], line[1], line[2]);
            g_strfreev(line);
        }
    }

    g_list_free_full(encounters, (GDestroyNotify)encounter_free);
    if (game_pf == NULL) {
        game_free(game);
    }

    return g_string_free(str, FALSE);
}

gchar *output_format_standings_plain(RinksTournament *tournament, RinksOutputType type, gint64 data, GList *teams_pf)
{
    gchar **standings = output_format_standings_raw(tournament, type, data, teams_pf);
    if (standings == NULL)
        return NULL;

    GString *str = g_string_new("");
    guint32 i = 0;
    while (standings[i] != NULL) {
        g_string_append_printf(str, "%.2s. %s\t%s\t%s\t%s\n", standings[i], standings[i+1],
                standings[i+2], standings[i+3], standings[i+4]);
        i += 5;
    }
    g_strfreev(standings);
    return g_string_free(str, FALSE);
} 

/* prefectched: if not NULL contains data which would be retrieved for this type */
gchar *output_format_plain(RinksTournament *tournament, RinksOutputType type, gint64 data, gpointer prefetched)
{
    switch (type) {
        case RinksOutputTypeRanking:
        case RinksOutputTypeRankingGroup:
            return output_format_standings_plain(tournament, type, data, prefetched);
        case RinksOutputTypeGameEncounter:
            return output_format_game_plain(tournament, data, (RinksGame *)prefetched);
    }
    return NULL;
}
