/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#pragma once

#include <glib.h>
#include "games.h"

typedef enum {
    ROUND_TYPE_NEXT_TO_GROUP,
    ROUND_TYPE_NEXT_TO_ALL,
    ROUND_TYPE_ROUND_ROBIN
} RinksRoundType;

typedef enum {
    RinksRoundFlagEncountersCreated = 1 << 0,
    RinksRoundFlagDone = 1 << 1,
    RinksRoundFlagAllowReencounters = 1 << 2
} RinksRoundFlag;

typedef struct {
    gint64 id;
    RinksRoundType type;
    gchar *description;
    gint range_start;
    gint range_end;
    guint flags;
    GList *games; /* [element-type: RinksGame] */
} RinksRound;

void round_free(RinksRound *round);

void rounds_create_encounters(gint64 round_id);
/* void rounds_create_games(gint64 round_id, RinksGameOrder order);*/

void rounds_update_encounters(void);
