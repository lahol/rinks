#include "rounds.h"
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
