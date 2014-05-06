#pragma once

#include <glib.h>
#include "tournament.h"

typedef enum {
    RinksOutputTypeGameEncounter, /* encounters of game in data */
    RinksOutputTypeRanking,
    RinksOutputTypeRankingGroup
} RinksOutputType;

void output_init(void);
void output_clear(void);
void output_add(RinksOutputType type, gint64 data);
gboolean output_print(RinksTournament *tournament, const gchar *filename);

/* prefectched: if not NULL contains data which would be retrieved for this type */
gchar *output_format(RinksTournament *tournament, RinksOutputType type, gint64 data, gpointer prefetched);
