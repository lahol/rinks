#include "tournament.h"
#include "data.h"
#include "db.h"

struct _RinksTournament {
    gpointer db_handle;
    gchar *filename;
    gchar *description;
    GList *rounds; /* [element-type: RinksRound] */
    GList *teams;  /* [element-type: RinksTeam] */
};

RinksTournament *tournament_create(gchar *filename)
{
}

RinksTournament *tournament_open(gchar *filename)
{
}

void tournament_close(RinksTournament *tournament)
{
}

void tournament_set_description(RinksTournament *tournament, gchar *description)
{
}

const gchar *tournament_get_description(RinksTournament *tournament)
{
}
