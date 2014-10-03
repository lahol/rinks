/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#include "games.h"

void game_free(RinksGame *game)
{
    if (game)
        g_free(game->description);
    g_free(game);
}
