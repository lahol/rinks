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
} RinksTeam;

typedef enum {
    ROUND_TYPE_NEXT_TO_GROUP,
    ROUND_TYPE_NEXT_TO_ALL,
    ROUND_TYPE_ROUND_ROBIN
} RinksRoundType;

typedef struct {
} RinksEncounter;

typedef struct {
    GList *encounters; /* [element-type: RinksEncounter] */
} RinksGame;

typedef struct {
    RinksRoundType type;
    gchar *description;
    GList *games; /* [element-type: RinksGame] */
} RinksRound;
