#pragma once

#include <glib.h>
#include "data.h"
#include "teams.h"
#include "rounds.h"

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
