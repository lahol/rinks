#include "application.h"

RinksTournament *current_tournament = NULL;

void application_set_current_tournament(RinksTournament *tournament)
{
    current_tournament = tournament;
}

RinksTournament *application_get_current_tournament(void)
{
    return current_tournament;
}
