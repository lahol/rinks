#pragma once

#include <glib.h>
#include "data.h"

gint db_init_database(gchar *path);
gint db_open_database(gchar *path);
void db_cleanup(void);

void db_add_team(RinksTeam *team);

GList *db_get_teams(void);
