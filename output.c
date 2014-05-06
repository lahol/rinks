#include "output.h"

typedef struct {
    RinksOutputType type;
    gint64 data;
} RinksOutputData;

GList *output_data = NULL;

void output_init(void)
{
}

void output_clear(void)
{
}

void output_add(RinksOutputType type, gint64 data)
{
}

gboolean output_print(RinksTournament *tournament, const gchar *filename)
{
    return TRUE;
}

gchar *output_format_game(RinksTournament *tournament, gint64 data, gpointer prefetched)
{
    GString *str = g_string_new("Game\nRink A: <b>Team1</b> vs <b>Team2</b>\nnext game");
    return g_string_free(str, FALSE);
}

gchar *output_format_standings(RinksTournament *tournament, RinksOutputType type, gint64 data, gpointer prefetched)
{
    GString *str = g_string_new("Standings");
    return g_string_free(str, FALSE);
} 

/* prefectched: if not NULL contains data which would be retrieved for this type */
gchar *output_format(RinksTournament *tournament, RinksOutputType type, gint64 data, gpointer prefetched)
{
    switch (type) {
        case RinksOutputTypeRanking:
        case RinksOutputTypeRankingGroup:
            return output_format_standings(tournament, type, data, prefetched);
        case RinksOutputTypeGameEncounter:
            return output_format_game(tournament, data, prefetched);
    }
    return NULL;
}
