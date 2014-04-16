#pragma once

#include <glib.h>

typedef struct _RinksTournament RinksTournament;

RinksTournament *tournament_open(gchar *filename, gboolean clear);
void tournament_close(RinksTournament *tournament);

void tournament_set_description(RinksTournament *tournament, const gchar *description);
const gchar *tournament_get_description(RinksTournament *tournament);
