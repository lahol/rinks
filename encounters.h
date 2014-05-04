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

typedef struct {
    gint64 id;
    gint64 team;
    gint64 encounter;
    gint32 points;
    gint32 ends;
    gint32 stones;
} RinksResult;

void encounter_free(RinksEncounter *encounter);

typedef enum {
    RinksEncounterSortLogical,
    RinksEncounterSortRinks
} RinksEncounterSortMode;
/* group a:1 -> group a:3 -> group b:1 -> rank 1 -> rank 2*/
GList *encounters_sort(GList *encounters, RinksEncounterSortMode mode);

gchar *encounters_translate(RinksEncounter *encounter);

gboolean encounters_encounter_parse_abstract_team(RinksEncounter *encounter, gint team, gint32 *group, gint32 *pos);
GList *encounters_filter_group(GList *encounters, gint32 group);
