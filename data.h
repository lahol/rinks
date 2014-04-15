#pragma once

#include <glib/glib.h>

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

typedef struct {
    gchar *description;
    GList *rounds; /* [element-type: RinksRound] */
} RinksTournament;
