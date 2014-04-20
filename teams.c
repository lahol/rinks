#include "teams.h"

void team_free(RinksTeam *team)
{
    if (team == NULL)
        return;

    g_free(team->name);
    g_free(team->skip);
}
