#pragma once

#include <glib.h>
#include "data.h"
#include "tournament.h"

typedef struct {
    void (*handle_tournament_changed)(RinksTournament *);
} RinksActionCallbacks;

RinksTournament *action_new_tournament(void);
RinksTournament *action_open_tournament(void);

void action_set_callbacks(RinksActionCallbacks *callbacks);
