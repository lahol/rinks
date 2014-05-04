#pragma once

#include <glib.h>

typedef enum {
    RinksGameOrderFirsts,            /* let A1:A2, B1:B2, C1:C2, … play in one game */
    RinksGameOrderGroupwise,         /* let A1:A2, A3:A4, A5:A6, … play in one round */
    RinksGameOrderRounds             /* same as Groupwise, but complete list */
} RinksGameOrder;

typedef struct {
    gint64 id;
    gchar *description;
    gint32 sequence;
    gint32 closed;
} RinksGame;

void game_free(RinksGame *game);
