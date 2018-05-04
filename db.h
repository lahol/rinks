/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#pragma once

#include <glib.h>
#include "teams.h"
#include "rounds.h"
#include "games.h"
#include "encounters.h"

gpointer db_open_database(gchar *path, gboolean clear);
void db_close_database(gpointer db_handle);

void db_set_property(gpointer db_handle, const gchar *key, const gchar *value);
gchar *db_get_property(gpointer db_handle, const gchar *key);

GList *db_get_teams(gpointer db_handle);
RinksTeam *db_get_team(gpointer db_handle, gint64 team_id);
gint64 db_add_team(gpointer db_handle, RinksTeam *team);
void db_update_team(gpointer db_handle, RinksTeam *team);
gint db_get_team_count(gpointer db_handle);

GList *db_get_rounds(gpointer db_handle);
RinksRound *db_get_round(gpointer db_handle, gint64 round_id);
gint64 db_add_round(gpointer db_handle, RinksRound *round);
void db_update_round(gpointer db_handle, RinksRound *round);
void db_round_set_flag(gpointer db_handle, gint64 round_id, guint flag);
void db_round_unset_flag(gpointer db_handle, gint64 round_id, guint flag);

gint64 db_add_encounter(gpointer db_handle, gint64 round_id,
                        const gchar *abstr_team1, const gchar *abstr_team2);
gint64 db_add_encounter_full(gpointer db_handle, gint64 round_id,
                             const gchar *abstr_team1, const gchar *abstr_team2,
                             gint64 real_team1, gint64 real_team2);
void db_update_encounter(gpointer db_handle, RinksEncounter *encounter);
GList *db_get_encounters(gpointer db_handle, gint64 round_id, gint64 game_id);
RinksEncounter *db_get_encounter(gpointer db_handle, gint64 encounter_id);
gboolean db_existed_encounter_before(gpointer db_handle, gint64 round_id, gint64 team1, gint64 team2);
void db_encounter_set_game(gpointer db_handle, gint64 encounter_id, gint64 game_id);

gint64 db_add_game(gpointer db_handle, RinksGame *game);
GList *db_get_games(gpointer db_handle);
RinksGame *db_get_game(gpointer db_handle, gint64 game_id);
void db_update_game(gpointer db_handle, RinksGame *game);

void db_set_result(gpointer db_handle, RinksResult *result);
RinksResult *db_get_result(gpointer db_handle, gint64 encounter, gint64 team);
GList *db_get_team_results(gpointer db_handle, gint64 team);

gint64 db_add_override(gpointer db_handle, RinksOverride *override);
void db_update_override(gpointer db_handle, RinksOverride *override);
GList *db_get_overrides(gpointer db_handle);
