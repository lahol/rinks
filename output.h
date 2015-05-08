/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#pragma once

#include <glib.h>
#include "tournament.h"

typedef enum {
    RinksOutputTypeRanking,
    RinksOutputTypeRankingGroup,
    RinksOutputTypeRoundEncounter, 
    RinksOutputTypeGameEncounter, /* encounters of game in data */
    RinksOutputTypeNone
} RinksOutputType;

void output_init(void);
void output_clear(void);
void output_add(RinksOutputType type, gint64 data);
gboolean output_print(RinksTournament *tournament, const gchar *filename);

/* prefectched: if not NULL contains data which would be retrieved for this type */
gchar *output_format_plain(RinksTournament *tournament, RinksOutputType type, gint64 data, gpointer prefetched);
