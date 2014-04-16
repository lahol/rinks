#pragma once

#include <glib.h>
#include "data.h"

gpointer db_open_database(gchar *path, gboolean clear);
void db_close_database(gpointer db_handle);

void db_set_property(gpointer db_handle, const gchar *key, const gchar *value);
const gchar *db_get_property(gpointer db_handle, const gchar *key);

void db_add_team(gpointer db_handle, RinksTeam *team);

GList *db_get_teams(gpointer db_handle);
