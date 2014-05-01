#pragma once

#include <glib.h>
#include "data.h"
#include "teams.h"
#include "rounds.h"
#include "encounters.h"

typedef struct _RinksTournament RinksTournament;

RinksTournament *tournament_open(gchar *filename, gboolean clear);
void tournament_close(RinksTournament *tournament);

void tournament_set_description(RinksTournament *tournament, const gchar *description);
const gchar *tournament_get_description(RinksTournament *tournament);

void tournament_set_rink_count(RinksTournament *tournament, gint rink_count);
gint tournament_get_rink_count(RinksTournament *tournament);

void tournament_set_group_count(RinksTournament *tournament, gint group_count);
gint tournament_get_group_count(RinksTournament *tournament);

void tournament_set_property(RinksTournament *tournament, const gchar *key, const gchar *value);
gchar *tournament_get_property(RinksTournament *tournament, const gchar *key);

void tournament_write_data(RinksTournament *tournament);

GList *tournament_get_teams(RinksTournament *tournament);
RinksTeam *tournament_get_team(RinksTournament *tournament, gint64 team_id);
gint64 tournament_add_team(RinksTournament *tournament, RinksTeam *team);
void tournament_update_team(RinksTournament *tournament, RinksTeam *team);
gint tournament_get_team_count(RinksTournament *tournament);

GList *tournament_get_rounds(RinksTournament *tournament);
RinksRound *tournament_get_round(RinksTournament *tournament, gint64 round_id);
gint64 tournament_add_round(RinksTournament *tournament, RinksRound *round);
void tournament_update_round(RinksTournament *tournament, RinksRound *round);
void tournament_round_set_flag(RinksTournament *tournament, gint64 round_id, guint flag);
void tournament_round_unset_flag(RinksTournament *tournament, gint64 round_id, guint flag);

gint64 tournament_add_encounter(RinksTournament *tournament, gint64 round_id,
                                const gchar *abstr_team1, const gchar *abstr_team2);
void tournament_update_encounter(RinksTournament *tournament, RinksEncounter *encounter);
GList *tournament_get_encounters(RinksTournament *tournament, gint64 round_id);
gboolean tournament_existed_encounter_before(RinksTournament *tournament, gint64 round_id,
                                             gint64 team1, gint64 team2);
