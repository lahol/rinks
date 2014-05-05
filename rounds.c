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
    GList *es = encounters_sort(g_list_copy(encounters), RinksEncounterSortLogical);
    
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
        g_printf("map encounter: %" G_GINT64_FORMAT ": %" G_GINT64_FORMAT " vs %" G_GINT64_FORMAT "\n",
                ((RinksEncounter *)tmp->data)->id, team_ids[i], team_ids[i + 1]);
        tournament_update_encounter(tournament, (RinksEncounter *)tmp->data);
    }
}

void rounds_create_games_for_encounters(RinksTournament *tournament, RinksRound *round,
                                        GList *teams, GList *encounters, RinksGameOrder order)
{
#if 0
    gint64 game_id;
    GList *tmp;
    gint32 rinkcount = tournament_get_rink_count(tournament);
    gint32 cur_rink = rinkcount;

    gint game_count = 0;
    RinksGame game;

    game.closed = 0;
    game.sequence = 0;

    for (tmp = encounters; tmp != NULL; tmp = g_list_next(tmp)) {
        if (++cur_rink > rinkcount) {
            game.description = g_strdup_printf("%s, Spiel %d", round->description, ++game_count);
            game_id = tournament_add_game(tournament, &game);
            g_free(game.description);

            cur_rink = 1;
        }

        ((RinksEncounter *)tmp->data)->game = game_id;
        tournament_update_encounter(tournament, (RinksEncounter *)tmp->data);
    }
#endif
}

void rounds_create_games(gint64 round_id, RinksGameOrder order)
{
#if 0
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

    encounters = tournament_get_encounters(tournament, round->id, 0);
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
#endif
}

gint rounds_compare_encounter_before(RinksEncounter *encounter, RinksEncounter *comp)
{
    if (encounter == NULL || comp == NULL)
        return 1;
    if (((encounter->real_team1 == comp->real_team1 &&
                encounter->real_team2 == comp->real_team2) ||
            (encounter->real_team1 == comp->real_team2 &&
             encounter->real_team2 == comp->real_team1)) &&
            encounter->round < comp->round)
        return 0;

    return 1;
}

gboolean rounds_existed_encounter_before(GList *encounters, gint64 team1, gint64 team2, gint64 current_round)
{
    RinksEncounter comp;
    comp.real_team1 = team1;
    comp.real_team2 = team2;
    comp.round = current_round;
    g_printf("encounter %" G_GINT64_FORMAT " vs %" G_GINT64_FORMAT " existed before %" G_GINT64_FORMAT,
            team1, team2, current_round);
    if (g_list_find_custom(encounters, &comp, (GCompareFunc)rounds_compare_encounter_before) != NULL) {
        g_printf(" TRUE\n");
        return TRUE;
    }

    g_printf(" FALSE\n");
    return FALSE;
}

void rounds_get_standings_before_round(GList *teams, GList *results, gint64 round_id)
{
    GList *t;
    GList *r;
    RinksTeam *team;
    RinksResult *result;

    for (t = teams; t != NULL; t = g_list_next(t)) {
        team = (RinksTeam *)t->data;
        team->points = 0;
        team->ends = 0;
        team->stones = 0;
        for (r = results; r != NULL; r = g_list_next(r)) {
            result = (RinksResult *)r->data;
            if (result->round < round_id && result->team == team->id) {
                team->points += result->points;
                team->ends += result->ends;
                team->stones += result->stones;
            }
        }
    }
}

guint32 rounds_find_id_in_array(gint64 *array, gint64 id, guint32 len)
{
    guint32 i;
    for (i = 0; i < len; ++i) {
        if (array[i] == id)
            return i;
    }

    return len;
}

/* teams contains the list of teams with the current standings before that round */
void rounds_map_encounters_for_round(RinksTournament *tournament, GList *map_teams, GList *map_encounters,
        RinksRound *round, GList *all_teams, GList *all_encounters)
{
    guint32 nteams = g_list_length(map_teams);
    guint32 nencounters = g_list_length(map_encounters);

    if (nteams != 2 * nencounters)
        g_printf("error: number of teams and encounters not matching\n");
    gint64 *teams = g_malloc(sizeof(gint64) * nteams);

    guint32 i, j;
    GList *tmp;

    for (i = 0, tmp = map_teams; tmp != NULL && i < nteams; ++i, tmp = g_list_next(tmp)) {
        teams[i] = ((RinksTeam *)tmp->data)->id;
        g_printf("rd %" G_GINT64_FORMAT " team %" G_GINT64_FORMAT ": %d %d %d\n",
                round->id, ((RinksTeam *)tmp->data)->id,
                ((RinksTeam *)tmp->data)->points,
                ((RinksTeam *)tmp->data)->ends,
                ((RinksTeam *)tmp->data)->stones);
    }
/*    for (i = 0, tmp = map_encounters; tmp != NULL && i < nencounters; ++i, tmp = g_list_next(tmp)) {
        encounters_encounter_parse_abstract_team(((RinksEncounter *)tmp->data), 1, NULL, &pos);
        if (pos >= 1 && pos <= nteams)
            encounters[i].real_team1 = teams[pos - 1];
        encounters_encounter_parse_abstract_team(((RinksEncounter *)tmp->data), 2, NULL, &pos);
        if (pos >= 1 && pos <= nteams)
            encounters[i].real_team2 = teams[pos - 1];
    }*/

    /* do some magic rearranging */
    gboolean conflict = FALSE;
    if (!(round->flags & RinksRoundFlagAllowReencounters)) {
        g_printf("before first pass (round %" G_GINT64_FORMAT ")\n", round->id);
        for (i = 0; i < nteams; ++i)
            g_printf(" %" G_GINT64_FORMAT, teams[i]);
        for (i = 0; i < nteams - 1; i += 2) {
            j = 1;
            while (i + j < nteams &&
                rounds_existed_encounter_before(all_encounters,
                        teams[i], teams[i + j], round->id)) {
                ++j;
            }
            if (i + j == nteams) {
                g_printf("conflict for pos %d\n", i);
                conflict = TRUE;
                break;
            }
            else {
                g_printf("move %d to %d\n", i + 1, i + j);
                util_array_move_element(teams, i + j, i + 1);
            }
        }
        g_printf("\nbefore second pass\n");
        for (i = 0; i < nteams; ++i)
            g_printf(" %" G_GINT64_FORMAT, teams[i]);
        for (i = nteams - 1; i > 0; i -= 2) {
            j = 1;
            while (i >= j &&
                    rounds_existed_encounter_before(all_encounters,
                        teams[i], teams[i - j], round->id)) {
                ++j;
            }
            if (j > i) {
                g_printf("\nerror: could not find matching\n");
            }
            else {
                util_array_move_element(teams, i - j, i - 1);
            }
            if (i == 1)
                break;
        }
        g_printf("\nafter last pass\n");
        for (i = 0; i < nteams; ++i)
            g_printf(" %" G_GINT64_FORMAT, teams[i]);
        g_printf("\n");
    }

    /* write encounters back */
    GList *tmp_global = all_encounters;
    GList *search_start;
    gboolean found_global;
    for (i = 0, tmp = map_encounters; tmp != NULL && i < nteams; i += 2, tmp = g_list_next(tmp)) {
        /* TODO: check if this works correctly */
        ((RinksEncounter *)tmp->data)->real_team1 = teams[i];
        ((RinksEncounter *)tmp->data)->real_team2 = teams[i + 1];

        search_start = tmp_global;
        found_global = FALSE;
        while (tmp_global) {
            if (((RinksEncounter *)tmp_global->data)->id == ((RinksEncounter *)tmp->data)->id) {
                found_global = TRUE;
                break;
            }
            tmp_global = g_list_next(tmp_global);
        }
        if (tmp_global == NULL) {
            tmp_global = all_encounters;
            while (!found_global && tmp_global && tmp_global != search_start) {
                if (((RinksEncounter *)tmp_global->data)->id == ((RinksEncounter *)tmp->data)->id) {
                    found_global = TRUE;
                    break;
                }
                tmp_global = g_list_next(tmp_global);
            }
        }
        if (found_global) {
            g_printf("encounter %d found global\n", i/2);
            ((RinksEncounter *)tmp_global->data)->real_team1 = teams[i];
            ((RinksEncounter *)tmp_global->data)->real_team2 = teams[i + 1];
        }
        else
            g_printf("encounter %d did not find global\n", i/2);
    }
}

void rounds_update_encounters(void)
{
    RinksTournament *tournament = application_get_current_tournament();
    if (tournament == NULL)
        return;

    GList *teams = tournament_get_teams(tournament);
    GList *results = tournament_get_team_results(tournament, 0);
    GList *rounds = tournament_get_rounds(tournament);

    GList *r;

    GList *encounters_all = tournament_get_encounters(tournament, 0, 0);
    GList *encounters_round = NULL;
    GList *encounters_group = NULL;
    GList *teams_filtered = NULL;
    GList *teams_sliced = NULL;
    gint32 i, ngroups = tournament_get_group_count(tournament);

    for (r = rounds; r != NULL; r = g_list_next(r)) {
        rounds_get_standings_before_round(teams, results, ((RinksRound *)r->data)->id);
        
        encounters_round = tournament_get_encounters(tournament,
                ((RinksRound *)r->data)->id, 0);

        switch (((RinksRound *)r->data)->type) {
            case ROUND_TYPE_NEXT_TO_GROUP:
                for (i = 1; i <= ngroups; ++i) {
                    teams_filtered = teams_sort(teams_filter(teams, RinksTeamFilterTypeGroup, GINT_TO_POINTER(i)),
                            RinksTeamSortGroup);
                    teams_sliced = teams_get_range(teams_filtered, ((RinksRound *)r->data)->range_start,
                            ((RinksRound *)r->data)->range_end - ((RinksRound *)r->data)->range_start + 1);
                    g_list_free(teams_filtered);
                    encounters_group = encounters_sort(encounters_filter_group(encounters_round, i),
                            RinksEncounterSortLogical);

                    rounds_map_encounters_for_round(tournament, teams_sliced, encounters_group,
                            (RinksRound *)r->data, teams, encounters_all);

                    g_list_free(teams_sliced);
                    g_list_free(encounters_group);
                }
                break;
            case ROUND_TYPE_NEXT_TO_ALL:
                teams = teams_sort(teams, RinksTeamSortAll);
                teams_sliced = teams_get_range(teams, ((RinksRound *)r->data)->range_start,
                        ((RinksRound *)r->data)->range_end - ((RinksRound *)r->data)->range_start + 1);
                encounters_round = encounters_sort(encounters_round, RinksEncounterSortLogical);
                rounds_map_encounters_for_round(tournament, teams_sliced, encounters_round,
                        (RinksRound *)r->data, teams, encounters_all);
                g_list_free(teams_sliced);
                break;
            case ROUND_TYPE_ROUND_ROBIN:
                break;
        }

        g_list_free_full(encounters_round, (GDestroyNotify)encounter_free);
    }

    GList *tmp;
    for (tmp = encounters_all; tmp != NULL; tmp = g_list_next(tmp)) {
        tournament_update_encounter(tournament, ((RinksEncounter *)tmp->data));
    }

    g_list_free_full(teams, (GDestroyNotify)team_free);
    g_list_free_full(results, g_free);
    g_list_free_full(rounds, (GDestroyNotify)round_free);
    g_list_free_full(encounters_all, (GDestroyNotify)encounter_free);
}
