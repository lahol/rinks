#include "teams.h"

void teams_free(RinksTeam *team)
{
    if (team == NULL)
        return;

    g_free(team->name);
    g_free(team->skip);
}
