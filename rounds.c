#include "rounds.h"

void round_free(RinksRound *round)
{
    if (round == NULL)
        return;

    g_free(round->description);
}
