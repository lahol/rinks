#pragma once

#include <glib.h>

typedef struct {
    guint32 id;
    guint32 group_id;
    gchar *name;
    gchar *skip;
    guint32 points;
    guint32 ends;
    guint32 stones;
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

typedef struct {
    gchar *description;
    GList *rounds; /* [element-type: RinksRound] */
    GList *teams;  /* [element-type: RinksTeam] */
} RinksTournament;
