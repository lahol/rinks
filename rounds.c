/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#include "rounds.h"
#include "games.h"
#include "encounters.h"
#include "tournament.h"
#include "application.h"

#include <glib/gprintf.h>
#include <memory.h>

struct RoundMatchingStackEntry {
    gint64 *matching;
    guint32 current[2];
};

void round_free(RinksRound *round)
{
    if (round == NULL)
        return;

    g_free(round->description);
}

struct RoundMatchingStackEntry *round_matching_stack_entry_new(gint64 *current, guint32 len,
                                                                guint32 first, guint32 second)
{
    struct RoundMatchingStackEntry *entry = g_malloc(sizeof(struct RoundMatchingStackEntry));
    entry->matching = g_malloc(sizeof(gint64) * len);
    memcpy(entry->matching, current, sizeof(gint64) * len);
    entry->current[0] = first;
    entry->current[1] = second;

    return entry;
}

void round_matching_stack_entry_free(struct RoundMatchingStackEntry *entry)
{
    if (entry != NULL) {
        g_free(entry->matching);
        g_free(entry);
    }
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

    g_list_free_full(teams, (GDestroyNotify)team_free);
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

void rounds_create_encounters_round_robin(RinksTournament *tournament, RinksRound *round)
{
    GList *teams = tournament_get_teams(tournament);
    teams = teams_sort(teams, RinksTeamSortGroupNoStanding);

    GList *team1, *team2;
    gchar *abstr_team1 = NULL;
    gchar *abstr_team2 = NULL;

    team2 = teams;

    gint32 p1, p2;
    gint32 current_group;
    gint64 id;

    g_printf("rounds create encounters round robin\n");
    while (team2) {
        current_group = ((RinksTeam *)team2->data)->group_id;
        g_printf("team2: %p, cur group: %u\n", team2, current_group);
        for (team1 = team2, p1 = 1;
             team1 && ((RinksTeam *)team1->data)->group_id == current_group;
             team1 = g_list_next(team1), ++p1) {
            g_printf("team1: %p, p1: %u\n", team1, p1);
            for (team2 = g_list_next(team1), p2 = p1 + 1;
                 team2 && ((RinksTeam *)(team2->data))->group_id == ((RinksTeam *)(team1->data))->group_id;
                 team2 = g_list_next(team2), ++p2) {
                g_printf("team2: %p, p2: %u\n", team2, p2);
                abstr_team1 = g_strdup_printf("rr%d:%d", ((RinksTeam *)team1->data)->group_id, p1);
                abstr_team2 = g_strdup_printf("rr%d:%d", ((RinksTeam *)team2->data)->group_id, p2);
                g_printf("add encounter %lld, %s vs %s\n", round->id, abstr_team1, abstr_team2);
                id = tournament_add_encounter_full(tournament, round->id, abstr_team1, abstr_team2,
                        ((RinksTeam *)team1->data)->id, ((RinksTeam *)team2->data)->id);
                g_free(abstr_team1);
                g_free(abstr_team2);
            }
        }
    }

    g_list_free_full(teams, (GDestroyNotify)team_free);
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
            rounds_create_encounters_round_robin(tournament, round);
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
        array[to] = t;
    }
    else {
        t = array[from];
        for (i = from; i < to; ++i)
            array[i] = array[i + 1];
        array[to] = t;
    }
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
    if (g_list_find_custom(encounters, &comp, (GCompareFunc)rounds_compare_encounter_before) != NULL) {
        return TRUE;
    }

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

gboolean rounds_does_override_exist(GList *overrides, gint64 round, gint64 team1, gint64 team2)
{
    if (team1 <= 0 || team2 <= 0)
        return FALSE;
    GList *tmp;
    for (tmp = overrides; tmp != NULL; tmp = g_list_next(tmp)) {
        if (((RinksOverride *)tmp->data)->round == round && 
                ((((RinksOverride *)tmp->data)->team1 == team1 &&
                  ((RinksOverride *)tmp->data)->team2 == team2) ||
                 (((RinksOverride *)tmp->data)->team1 == team2 &&
                  ((RinksOverride *)tmp->data)->team2 == team1))) {
            return TRUE;
        }
    }
    return FALSE;
}

RinksOverride *rounds_get_override(GList *override, gint64 encounter)
{
    GList *tmp;
    for (tmp = override; tmp != NULL; tmp = g_list_next(tmp)) {
        if (((RinksOverride *)tmp->data)->encounter == encounter)
            return (RinksOverride *)tmp->data;
    }

    return NULL;
}

void rounds_set_overrides(gint64 round, GList *overrides, GList *encounters, gint64 *teams, guint32 *nteams)
{
    GList *tmp;
    RinksOverride *override;
    guint32 i, j;

    guint32 newlen = *nteams;

    for (tmp = encounters; tmp != NULL; tmp = g_list_next(tmp)) {
        if (((RinksEncounter *)tmp->data)->round == round) {
            override = rounds_get_override(overrides, ((RinksEncounter *)tmp->data)->id);
            if (override && override->team1 > 0 && override->team2 > 0 &&
                    override->team1 != override->team2) {
                ((RinksEncounter *)tmp->data)->real_team1 = override->team1;
                ((RinksEncounter *)tmp->data)->real_team2 = override->team2;
                /* set array elements to zero */
                i = rounds_find_id_in_array(teams, override->team1, *nteams);
                j = rounds_find_id_in_array(teams, override->team2, *nteams);
                if (i != *nteams && j != *nteams) {
                    teams[i] = teams[j] = 0;
                    newlen -= 2;
                }
            }
        }
    }

    if (newlen < *nteams) {
        /* compress array */
        j = 0;
        for (i = 0; i + j < *nteams; ++i) {
            while (i + j < *nteams && teams[i + j] == 0) { ++j; }
            if (i + j == *nteams)
                break;
            if (j > 0) {
                teams[i] = teams[i + j];
            }
        }
        *nteams = newlen;
    }
}

/* teams contains the list of teams with the current standings before that round */
void rounds_map_encounters_for_round(RinksTournament *tournament, GList *map_teams, GList *map_encounters,
        RinksRound *round, GList *all_teams, GList *all_encounters, GList *overrides)
{
    guint32 nteams = g_list_length(map_teams);
    guint32 nencounters = g_list_length(map_encounters);

    if (nteams != 2 * nencounters && round->type != ROUND_TYPE_ROUND_ROBIN)
        g_printf("error: number of teams and encounters not matching (%u vs %u)\n", nteams, 2 * nencounters);
    gint64 *teams = g_malloc(sizeof(gint64) * nteams);
    gint64 *teams_orig = g_malloc(sizeof(gint64) * nteams);
    gint64 *teams_result = NULL, *current = NULL;
    guint32 nteams_orig = nteams;

    GQueue *stack = g_queue_new();
    struct RoundMatchingStackEntry *stack_entry = NULL;

    guint32 i, j;
    GList *tmp;

    for (i = 0, tmp = map_teams; tmp != NULL && i < nteams; ++i, tmp = g_list_next(tmp)) {
        teams_orig[i] = teams[i] = ((RinksTeam *)tmp->data)->id;
/*        g_printf("rd %" G_GINT64_FORMAT " team %" G_GINT64_FORMAT ": %d %d %d\n",
                round->id, ((RinksTeam *)tmp->data)->id,
                ((RinksTeam *)tmp->data)->points,
                ((RinksTeam *)tmp->data)->ends,
                ((RinksTeam *)tmp->data)->stones);*/
    }

    /* shrink by overridden encounters */
    rounds_set_overrides(round->id, overrides, map_encounters, teams, &nteams);

    if (nteams == 0) {
        goto out;
    }

    teams_result = g_malloc(sizeof(gint64) * nteams);
    current = g_malloc(sizeof(gint64) * nteams);
    memcpy(teams_result, teams, sizeof(gint64) * nteams);
    memcpy(current, teams, sizeof(gint64) * nteams);

    gboolean conflict = FALSE;
    for (i = 0; i < nteams; i += 2) {
        j = i + 1;
        if (!(round->flags & RinksRoundFlagAllowReencounters)) {
            while (j < nteams &&
                    rounds_existed_encounter_before(all_encounters,
                        current[i], current[j], round->id)) {
                ++j;
            }
            if (j == nteams) {
                conflict = TRUE;
                break;
            }
        }
        stack_entry = round_matching_stack_entry_new(current, nteams, i, j);
        g_queue_push_head(stack, stack_entry);
        util_array_move_element(current, j, i + 1);
        /* set next encounter correctly */
        j = i + 3; /*????*/
    }

    if (!conflict) {
        memcpy(teams_result, current, sizeof(gint64) * nteams);
    }
    else {
        while (!g_queue_is_empty(stack)) {
            stack_entry = (struct RoundMatchingStackEntry *)g_queue_pop_head(stack);
            memcpy(current, stack_entry->matching, sizeof(gint64) * nteams);
            i = stack_entry->current[0];
            j = stack_entry->current[1] + 1;
            round_matching_stack_entry_free(stack_entry);
            
            conflict = FALSE;
            for ( ; i < nteams; i += 2) {
                while (j < nteams &&
                        rounds_existed_encounter_before(all_encounters,
                            current[i], current[j], round->id)) {
                    ++j;
                }
                if (j == nteams) {
                    conflict = TRUE;
                    break;
                }
                stack_entry = round_matching_stack_entry_new(current, nteams, i, j);
                g_queue_push_head(stack, stack_entry);
                util_array_move_element(current, j, i + 1);
                j = i + 3;
            }

            if (!conflict) {
                memcpy(teams_result, current, sizeof(gint64) * nteams);
                break;
            }
        }
    }

    /* write encounters back */
    RinksOverride *ovr;
    for (i = 0, tmp = map_encounters; tmp != NULL && i < nteams; i += 2, tmp = g_list_next(tmp)) {
        /* skip overridden encounters, use same validation as above */
        while (tmp && (ovr = rounds_get_override(overrides, ((RinksEncounter *)tmp->data)->id)) != NULL) {
            if (ovr->team1 > 0 && ovr->team2 && ovr->team1 != ovr->team2 &&
                    rounds_find_id_in_array(teams_orig, ovr->team1, nteams_orig) != nteams_orig &&
                    rounds_find_id_in_array(teams_orig, ovr->team2, nteams_orig) != nteams_orig) {
                tmp = g_list_next(tmp);
            }
        }
        if (tmp == NULL)
            break;

        ((RinksEncounter *)tmp->data)->real_team1 = teams_result[i];
        ((RinksEncounter *)tmp->data)->real_team2 = teams_result[i + 1];
    }
out:
    g_free(teams);
    g_free(teams_orig);
    g_free(teams_result);
    g_free(current);

    g_queue_free_full(stack, (GDestroyNotify)round_matching_stack_entry_free);
}

void rounds_update_encounters(void)
{
    g_printf("rounds_update_encounters\n");
    RinksTournament *tournament = application_get_current_tournament();
    if (tournament == NULL)
        return;

    GList *teams = tournament_get_teams(tournament);
    GList *results = tournament_get_team_results(tournament, 0);
    GList *rounds = tournament_get_rounds(tournament);
    GList *overrides = tournament_get_overrides(tournament);

    GList *r;

    GList *encounters_all = tournament_get_encounters(tournament, 0, 0);
    GList *encounters_round = NULL;
    GList *encounters_group = NULL;
    GList *teams_filtered = NULL;
    GList *teams_sliced = NULL;
    gint32 i, ngroups = tournament_get_group_count(tournament);

    for (r = rounds; r != NULL; r = g_list_next(r)) {
        rounds_get_standings_before_round(teams, results, ((RinksRound *)r->data)->id);
        
        encounters_round = encounters_filter(encounters_all, RinksEncounterFilterTypeRound, ((RinksRound *)r->data)->id);

        switch (((RinksRound *)r->data)->type) {
            case ROUND_TYPE_NEXT_TO_GROUP:
                for (i = 1; i <= ngroups; ++i) {
                    teams_filtered = teams_sort(teams_filter(teams, RinksTeamFilterTypeGroup, GINT_TO_POINTER(i)),
                            RinksTeamSortGroup);
                    teams_sliced = teams_get_range(teams_filtered, ((RinksRound *)r->data)->range_start,
                            ((RinksRound *)r->data)->range_end - ((RinksRound *)r->data)->range_start + 1);
                    g_list_free(teams_filtered);
                    encounters_group = encounters_sort(encounters_filter(encounters_round, RinksEncounterFilterTypeGroup, (gint64)i),
                            RinksEncounterSortLogical);

                    rounds_map_encounters_for_round(tournament, teams_sliced, encounters_group,
                            (RinksRound *)r->data, teams, encounters_all, overrides);

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
                        (RinksRound *)r->data, teams, encounters_all, overrides);
                g_list_free(teams_sliced);
                break;
            case ROUND_TYPE_ROUND_ROBIN:
/*                teams = teams_sort(teams, RinksTeamSortGroupNoStanding);
                g_printf("Map encounters for round robin\n");*/
                /* nothing to map */
                break;
        }

        g_list_free(encounters_round);
    }

    GList *tmp;
    for (tmp = encounters_all; tmp != NULL; tmp = g_list_next(tmp)) {
        tournament_update_encounter(tournament, ((RinksEncounter *)tmp->data));
    }

    g_list_free_full(overrides, g_free);
    g_list_free_full(teams, (GDestroyNotify)team_free);
    g_list_free_full(results, g_free);
    g_list_free_full(rounds, (GDestroyNotify)round_free);
    g_list_free_full(encounters_all, (GDestroyNotify)encounter_free);
}
