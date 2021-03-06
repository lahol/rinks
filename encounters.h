/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
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
    gint64 game;
    gint64 round;
} RinksResult;

typedef struct {
    gint64 id;
    gint64 encounter;
    gint64 round;
    gint64 team1;
    gint64 team2;
} RinksOverride;

void encounter_free(RinksEncounter *encounter);

typedef enum {
    RinksEncounterSortLogical,
    RinksEncounterSortRinks
} RinksEncounterSortMode;
/* group a:1 -> group a:3 -> group b:1 -> rank 1 -> rank 2*/
GList *encounters_sort(GList *encounters, RinksEncounterSortMode mode);

gchar *encounters_translate(RinksEncounter *encounter);

gboolean encounters_encounter_parse_abstract_team(RinksEncounter *encounter, gint team, gint32 *group, gint32 *pos);

typedef enum {
    RinksEncounterFilterTypeRound,
    RinksEncounterFilterTypeGroup
} RinksEncounterFilterType;

GList *encounters_filter(GList *encounters, RinksEncounterFilterType type, gint64 filter);
