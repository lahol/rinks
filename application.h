/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#pragma once

#include "tournament.h"

void application_set_current_tournament(RinksTournament *tournament);
RinksTournament *application_get_current_tournament(void);
