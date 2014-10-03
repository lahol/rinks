/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#pragma once

#include <glib.h>

typedef struct {
    gint64 id;
    gint32 group_id;
    gchar *name;
    gchar *skip;
    gint32 points;
    gint32 ends;
    gint32 stones;
    guint32 valid_keys;
} RinksTeam;

enum {
    RinksTeamKeyName = 1 << 0,
    RinksTeamKeySkip = 1 << 1,
    RinksTeamKeyGroupId = 1 << 2,
    RinksTeamKeyPoints = 1 << 3,
    RinksTeamKeyEnds = 1 << 4,
    RinksTeamKeyStones = 1 << 5,
};

void team_free(RinksTeam *team);

typedef enum {
    RinksTeamSortGroup,
    RinksTeamSortAll
} RinksTeamSortMode;

GList *teams_sort(GList *list, RinksTeamSortMode mode);

gint teams_compare_standings(RinksTeam *a, RinksTeam *b);

typedef enum {
    RinksTeamFilterTypeGroup
} RinksTeamFilterType;

/* creates a new list, but does not copy data, just references it */
GList *teams_filter(GList *teams, RinksTeamFilterType type, gpointer data);
/* offset is 1-based */
GList *teams_get_range(GList *teams, gint64 offset, gint64 count);
