#include "encounters.h"
#include <stdio.h>
#include <string.h>
#include <glib/gprintf.h>

void encounter_free(RinksEncounter *encounter)
{
    if (encounter == NULL)
        return;
    g_free(encounter->abstract_team1);
    g_free(encounter->abstract_team2);

    g_free(encounter);
}

gboolean encounters_encounter_parse_abstract_team(RinksEncounter *encounter, gint team, gint32 *group, gint32 *pos)
{
    if (encounter == NULL)
        return FALSE;
    gchar *str = NULL;
    if (team == 1)
        str = encounter->abstract_team1;
    else if (team == 2)
        str = encounter->abstract_team2;
    else
        return FALSE;
    if (str == NULL)
        return FALSE;

    gint32 g = 0, p = 0;
    int rc;

    if (strlen(str) < 3)
        return FALSE;
    if (str[0] == 'g' && str[1] == 'r') {
        rc = sscanf(str, "gr%d:%d", &g, &p);
        if (rc < 2)
            return FALSE;
    }
    else if (str[0] == 'r' && str[1] == 'k') {
        rc = sscanf(str, "rk%d", &p);
        if (rc < 1)
            return FALSE;
    }
    else {
        return FALSE;
    }

    if (group) *group = g;
    if (pos) *pos = p;

    return TRUE;
}

gint encounters_sort_compare_logical(RinksEncounter *a, RinksEncounter *b)
{
    if (a == NULL || b == NULL)
        return 0;
    if (a == NULL)
        return -1;
    if (b == NULL)
        return 1;

    gint32 ga, gb, pa, pb;

    if (a->round < b->round)
        return -1;
    else if (a->round > b->round)
        return 1;

    if (!encounters_encounter_parse_abstract_team(a, 1, &ga, &pa))
        return -1;
    if (!encounters_encounter_parse_abstract_team(b, 1, &gb, &pb))
        return 1;

    if (ga == gb && ga == 0) {
        if (pa < pb)
            return -1;
        else if (pa > pb)
            return 1;
        else
            return 0;
    }
    if (ga == 0)
        return 1;
    if (gb == 0)
        return -1;
    if (ga < gb)
        return -1;
    if (ga > gb)
        return 1;
    if (pa < pb)
        return -1;
    if (pa > pb)
        return 1;
    return 0;
}

gint encounters_sort_compare_rinks(RinksEncounter *a, RinksEncounter *b)
{
    if (a == NULL || b == NULL)
        return 0;
    if (a == NULL)
        return -1;
    if (b == NULL)
        return 1;

    if (a->rink < b->rink)
        return -1;
    else if (a->rink > b->rink)
        return 1;
    else
        return 0;
}

GList *encounters_sort(GList *encounters, RinksEncounterSortMode mode)
{
    switch (mode) {
        case RinksEncounterSortLogical:
            return g_list_sort(encounters, (GCompareFunc)encounters_sort_compare_logical);
        case RinksEncounterSortRinks:
            return g_list_sort(encounters, (GCompareFunc)encounters_sort_compare_rinks);
    }

    return NULL;
}

gchar *encounters_translate(RinksEncounter *encounter)
{
    gint32 group[2];
    gint32 pos[2];

    if (!encounters_encounter_parse_abstract_team(encounter, 1, &group[0], &pos[0]))
        return NULL;
    if (!encounters_encounter_parse_abstract_team(encounter, 2, &group[1], &pos[1]))
        return NULL;

    if (group[0] != group[1])
        return NULL;

    if (group[0] == 0)
        return g_strdup_printf("Runde %" G_GINT64_FORMAT ", %d. RL – %d. RL",
                encounter->round, pos[0], pos[1]);
    else
        return g_strdup_printf("Runde %" G_GINT64_FORMAT ", %d. Gr. %c – %d. Gr. %c",
                encounter->round, pos[0], (gchar)(group[0] - 1 + 'A'),
                pos[1], (gchar)(group[1] - 1 + 'A'));

}

GList *encounters_filter_group(GList *encounters, gint32 group)
{
    gint32 g1, g2;
    GList *filtered = NULL;
    GList *tmp;

    for (tmp = encounters; tmp != NULL; tmp = g_list_next(tmp)) {
        if (encounters_encounter_parse_abstract_team((RinksEncounter *)tmp->data,
                    1, &g1, NULL) &&
                encounters_encounter_parse_abstract_team((RinksEncounter *)tmp->data,
                    2, &g2, NULL) && g1 == g2 && g1 == group) {
            filtered = g_list_prepend(filtered, tmp->data);
        }
    }

    return g_list_reverse(filtered);
}
