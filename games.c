#include "games.h"

void game_free(RinksGame *game)
{
    if (game)
        g_free(game->description);
    g_free(game);
}
