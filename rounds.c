#include "rounds.h"
#include "games.h"
#include "encounters.h"
#include "tournament.h"
#include "application.h"

#include <glib/gprintf.h>

void round_free(RinksRound *round)
{
    if (round == NULL)
        return;

    g_free(round->description);
}

void rounds_create_encounters_group(RinksTournament *tournament, RinksRound *round)
{
    GList *teams = tournament_get_teams(tournament);
    teams = teams_sort(teams, RinksTeamSortGroup);

    GList *team1, *team2;
    gchar *abstr_team1 = NULL;
    gchar *abstr_team2 = NULL;

    team1 = teams;
    team2 = team1 ? g_list_next(team1) : NULL;

    gint32 last_group = 0;
    gint32 pos = 1;
    gint64 id;

    while (team1 && team2) {
        if (((RinksTeam *)team1->data)->group_id != ((RinksTeam *)team2->data)->group_id) {
            g_printf("error: odd number of teams in group\n");
            team1 = team2;
            team2 = g_list_next(team1);
            if (team2 == NULL)
                break;
        }

        if (last_group != ((RinksTeam *)team1->data)->group_id) {
            last_group = ((RinksTeam *)team1->data)->group_id;
            pos = 1;
        }

        abstr_team1 = g_strdup_printf("gr%d:%d", ((RinksTeam *)team1->data)->group_id, pos);
        abstr_team2 = g_strdup_printf("gr%d:%d", ((RinksTeam *)team2->data)->group_id, pos + 1);
        id = tournament_add_encounter(tournament, round->id, abstr_team1, abstr_team2);
        g_free(abstr_team1);
        g_free(abstr_team2);
        pos += 2;

        g_printf("added encounter %" G_GINT64_FORMAT "\n", id);

        team1 = g_list_next(team2);
        team2 = team1 ? g_list_next(team1) : NULL;
    }
    if (team1)
        g_printf("error: odd number of teams\n");
}

void rounds_create_encounters_all(RinksTournament *tournament, RinksRound *round)
{
    gint nteams = tournament_get_team_count(tournament);
    if (nteams % 2 == 1)
        g_printf("error: odd number of teams\n");

    gint i;
    gchar *abstr_team1, *abstr_team2;
    for (i = round->range_start; i <= round->range_end && i <= nteams; i += 2) {
        abstr_team1 = g_strdup_printf("rk%d", i);
        abstr_team2 = g_strdup_printf("rk%d", i + 1);
        tournament_add_encounter(tournament, round->id, abstr_team1, abstr_team2);
        g_free(abstr_team1);
        g_free(abstr_team2);
    }
}

void rounds_create_encounters(gint64 round_id)
{
    RinksTournament *tournament = application_get_current_tournament();
    RinksRound *round = tournament_get_round(tournament, round_id);

    if (round == NULL)
        return;

    switch (round->type) {
        case ROUND_TYPE_NEXT_TO_GROUP:
            rounds_create_encounters_group(tournament, round);
            break;
        case ROUND_TYPE_NEXT_TO_ALL:
            rounds_create_encounters_all(tournament, round);
            break;
        case ROUND_TYPE_ROUND_ROBIN:
            break;
    }

    tournament_round_set_flag(tournament, round_id, RinksRoundFlagEncountersCreated);

    round_free(round);
}

void util_array_move_element(gint64 *array, guint from, guint to)
{
    guint i;
    gint64 t;
    if (to == from)
        return;
    if (to < from) {
        t = array[from];
        for (i = from; i > to; --i)
            array[i] = array[i - 1];
        array[i] = t;
    }
    else {
        t = array[from];
        for (i = from; i < to; ++i)
            array[i] = array[i + 1];
        array[i] = t;
    }
}

/* only pass teams for that type of encounters e.g. only teams of one group */
void rounds_map_teams_to_encounters(RinksTournament *tournament, RinksRound *round, GList *teams, GList *encounters)
{
    if (round == NULL || teams == NULL || encounters == NULL)
        return;

    GList *ts = teams_sort(g_list_copy(teams), RinksTeamSortGroup);
    GList *es = encounters_sort(g_list_copy(encounters));
    
    /* TODO: do real mapping of abstract teams (e.g. a:2 vs b:3) 
     * for now: a:1 vs a:2, a:3 vs a:4, … expected */

    guint team_count = g_list_length(ts);
    gint64 *team_ids = g_malloc0(sizeof(gint64) * team_count);

    GList *tmp;
    guint i, j;

    for (i = 0, tmp = ts; tmp != NULL && i < team_count; tmp = g_list_next(tmp), ++i) {
        team_ids[i] = ((RinksTeam *)tmp->data)->id;
    }

    /* modify list so that no one sees the same encounter as before */
    /* TODO: switch with encounter above */
    /* first pass: from top to bottom: find next best; if no team left, stop, second pass
     * second pass: from bottom to top: find next worst; */
    if (!(round->flags & RinksRoundFlagAllowReencounters)) {
        gboolean conflict = FALSE;
        for (i = 0; i < team_count; i += 2) {
            j = 1;
            while (i + j < team_count &&
                    tournament_existed_encounter_before(tournament, round->id, team_ids[i], team_ids[i+j])) {
                ++j;
            }
            if (i + j == team_count) {
                conflict = TRUE;
                break;
            }
            else {
                util_array_move_element(team_ids, i + 1, i + j);
            }
        }
        for (i = team_count - 1; i > 0; i -= 2) {
            j = 1;
            while (i >= j &&
                    tournament_existed_encounter_before(tournament, round->id, team_ids[i], team_ids[i-j])) {
                ++j;
            }
            if (j > i) {
                g_printf("error: could not find matching\n");
            }
            else {
                util_array_move_element(team_ids, i - 1, i - j);
            }
            if (i == 1)
                break;
        }
        /* sort pairwise? [12453768]->[12374568] */
    }

    /* map to encounters */
    for (i = 0, tmp = es; i + 1 < team_count && tmp != NULL; tmp = g_list_next(tmp), i += 2) {
        ((RinksEncounter *)tmp->data)->real_team1 = team_ids[i];
        ((RinksEncounter *)tmp->data)->real_team2 = team_ids[i + 1];
        tournament_update_encounter(tournament, (RinksEncounter *)tmp->data);
    }
}

void rounds_create_games_for_encounters(RinksTournament *tournament, RinksRound *round,
                                        GList *teams, GList *encounters, RinksGameOrder order)
{
}

void rounds_create_games(gint64 round_id, RinksGameOrder order)
{
    /* 1. map teams to encounters (group intern, all) 
     * 2. pack encounters to games
     * 3. pack encounters of games to rinks */

    RinksTournament *tournament = application_get_current_tournament();
    RinksRound *round = tournament_get_round(tournament, round_id);

    GList *teams = NULL, *encounters = NULL;

    if (round == NULL)
        goto out;

    teams = tournament_get_teams(tournament);
    if (teams == NULL)
        goto out;

    GList *teams_filtered = NULL, *teams_sliced = NULL;

    encounters = tournament_get_encounters(tournament, round->id);
    if (encounters == NULL)
        goto out;

    GList *encounters_group = NULL;
    gint ngroups, i;


    switch (round->type) {
        case ROUND_TYPE_NEXT_TO_GROUP:
            ngroups = tournament_get_group_count(tournament);
            for (i = 1; i <= ngroups; ++i) {
                teams_filtered = teams_filter(teams, RinksTeamFilterTypeGroup, GINT_TO_POINTER(i));
                teams_sliced = teams_get_range(teams_filtered, round->range_start,
                        round->range_end - round->range_start + 1);
                g_list_free(teams_filtered);
                encounters_group = encounters_filter_group(encounters, i);

                rounds_map_teams_to_encounters(tournament, round, teams_sliced, encounters_group);

                g_list_free(teams_sliced);
                g_list_free(encounters_group);
            }
            break;
        case ROUND_TYPE_NEXT_TO_ALL:
            teams_sliced = teams_get_range(teams, round->range_start,
                    round->range_end - round->range_start + 1);
            rounds_map_teams_to_encounters(tournament, round, teams_sliced, encounters);
            g_list_free(teams_sliced);
            break;
        case ROUND_TYPE_ROUND_ROBIN:
            break;
    }

    rounds_create_games_for_encounters(tournament, round, teams, encounters, order);

out:
    if (round != NULL)
        round_free(round);
    g_list_free_full(encounters, (GDestroyNotify)encounter_free);
    g_list_free_full(teams, (GDestroyNotify)team_free);
}
