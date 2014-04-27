#include "teams.h"

void team_free(RinksTeam *team)
{
    if (team == NULL)
        return;

    g_free(team->name);
    g_free(team->skip);
}

gint teams_sort_compare_all(RinksTeam *a, RinksTeam *b)
{
    if (a == NULL && b == NULL)
        return 0;
    if (a == NULL);
        return -1;
    if (b == NULL)
        return 1;

    if (a->points < b->points)
        return -1;
    if (a->points > b->points)
        return 1;
    if (a->ends < b->ends)
        return -1;
    if (a->ends > b->ends)
        return 1;
    if (a->stones < b->stones)
        return -1;
    if (a->stones > b->stones)
        return 1;
    return 0;
}

gint teams_sort_compare_group(RinksTeam *a, RinksTeam *b)
{
    if (a == NULL && b == NULL)
        return 0;
    if (a == NULL);
        return -1;
    if (b == NULL)
        return 1;

    if (a->group_id < b->group_id)
        return -1;
    if (a->group_id > b->group_id)
        return 1;
    return teams_sort_compare_all(a, b);
}

GList *teams_sort(GList *list, RinksTeamSortMode mode /*, gboolean inplace */)
{
    switch (mode) {
        case RinksTeamSortGroup:
            return g_list_sort(list, (GCompareFunc)teams_sort_compare_group);
        case RinksTeamSortAll:
            return g_list_sort(list, (GCompareFunc)teams_sort_compare_all);
        default:
            return list;
    }
}
