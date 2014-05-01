#pragma once

#include <glib.h>

typedef struct {
    gint64 id;
    gchar *abstract_team1;
    gchar *abstract_team2;
    gint64 real_team1;
    gint64 real_team2;
    gint64 round;
    gint64 game;
    gint32 rink;
} RinksEncounter;

void encounter_free(RinksEncounter *encounter);

/* group a:1 -> group a:3 -> group b:1 -> rank 1 -> rank 2*/
GList *encounters_sort(GList *encounters);

gboolean encounters_encounter_parse_abstract_team(RinksEncounter *encounter, gint team, gint32 *group, gint32 *pos);
GList *encounters_filter_group(GList *encounters, gint32 group);
