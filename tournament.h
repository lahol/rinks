#pragma once

#include <glib.h>
#include "data.h"

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
gint64 tournament_add_team(RinksTournament *tournament, RinksTeam *team);
void tournament_update_team(RinksTournament *tournament, RinksTeam *team);
