#pragma once

#include <glib.h>

typedef struct {
} RinksEncounter;

typedef struct {
    GList *encounters; /* [element-type: RinksEncounter] */
} RinksGame;
