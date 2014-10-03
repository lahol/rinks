/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#pragma once

#include <glib.h>
#include "tournament.h"

typedef struct {
    void (*handle_tournament_changed)(RinksTournament *);
} RinksActionCallbacks;

RinksTournament *action_new_tournament(void);
RinksTournament *action_open_tournament(void);

void action_set_callbacks(RinksActionCallbacks *callbacks);
