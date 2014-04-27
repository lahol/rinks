#pragma once

#include "data.h"

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
