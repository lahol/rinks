#include "output.h"
#include <glib/gprintf.h>
#include <pango/pangocairo.h>
#include <cairo.h>
#include <cairo-pdf.h>
#include <time.h>

#define CAIRO_MM_PTS   (3.53)

typedef struct {
    RinksOutputType type;
    gint64 data;
} RinksOutputData;

GList *output_data = NULL;

void output_init(void)
{
    output_clear();
}

void output_clear(void)
{
    g_list_free_full(output_data, g_free);
    output_data = NULL;
}

void output_add(RinksOutputType type, gint64 data)
{
    RinksOutputData *entry = g_malloc0(sizeof(RinksOutputData));
    entry->type = type;
    entry->data = data;

    output_data = g_list_prepend(output_data, entry);
}

gchar **output_format_game_encounter(RinksTournament *tournament, RinksEncounter *encounter)
{
    g_return_val_if_fail(tournament != NULL, NULL);
    g_return_val_if_fail(encounter != NULL, NULL);

    RinksTeam *teams[2];
    teams[0] = tournament_get_team(tournament, encounter->real_team1);
    teams[1] = tournament_get_team(tournament, encounter->real_team2);

    gchar **output = g_malloc0(sizeof(gchar *) * 4);

    output[0] = g_strdup_printf("Rink %c", encounter->rink > 0 ? (gchar)(encounter->rink - 1 + 'A') : '-');
    output[1] = teams[0] != NULL ? g_strdup(teams[0]->name) : g_strdup("???");
    output[2] = teams[1] != NULL ? g_strdup(teams[1]->name) : g_strdup("???");

    team_free(teams[0]);
    team_free(teams[1]);

    return output;
}

/* return nteams * 5 array: pos, name, pts, ends, stones */
gchar **output_format_standings_raw(RinksTournament *tournament, RinksOutputType type, gint64 data, GList *teams_pf)
{
    GList *teams_sorted = NULL;
    GList *teams_filtered = NULL;
    GList *teams = NULL;

    if (teams_pf != NULL) {
        teams_filtered = g_list_copy(teams_pf);
    }
    else {
        teams = tournament_get_teams(tournament);
        if (type == RinksOutputTypeRankingGroup) {
            teams_filtered = teams_filter(teams, RinksTeamFilterTypeGroup, GINT_TO_POINTER((gint32)data));
        }
        else {
            teams_filtered = g_list_copy(teams);
        }
    }

    teams_sorted = teams_sort(teams_filtered, RinksTeamSortAll);

    RinksTeam *pred = NULL;
    GList *tmp;

    guint32 nteams = g_list_length(teams_sorted);
    guint32 i;

    gchar **output = g_malloc0(sizeof(gchar *) * (5 * nteams + 1));

    for (tmp = teams_sorted, i = 0; tmp != NULL && i < nteams; tmp = g_list_next(tmp), ++i) {
        output[5 * i + 1] = g_strdup(((RinksTeam *)tmp->data)->name);
        output[5 * i + 2] = g_strdup_printf("%d", ((RinksTeam *)tmp->data)->points);
        output[5 * i + 3] = g_strdup_printf("%d", ((RinksTeam *)tmp->data)->ends);
        output[5 * i + 4] = g_strdup_printf("%d", ((RinksTeam *)tmp->data)->stones);

        if (pred && teams_compare_standings(pred, ((RinksTeam *)tmp->data)) == 0) {
            output[5 * i] = g_strdup(output[5 * (i - 1)]);
        }
        else {
            output[5 * i] = g_strdup_printf("%" G_GUINT32_FORMAT , (i + 1));
        }

        pred = (RinksTeam *)tmp->data;
    }

    g_list_free(teams_filtered);
    g_list_free_full(teams, (GDestroyNotify)team_free);

    return output;
}

gchar *output_format_encounters_plain(RinksTournament *tournament, GList *encounters)
{
    GString *str = g_string_new("");
    GList *tmp;
    gchar **line = NULL;
    for (tmp = encounters; tmp != NULL; tmp = g_list_next(tmp)) {
        line = output_format_game_encounter(tournament, ((RinksEncounter *)tmp->data));
        if (line != NULL) {
            g_string_append_printf(str, "%s: %s\t–\t%s\n", line[0], line[1], line[2]);
            g_strfreev(line);
        }
    }

    return g_string_free(str, FALSE);
}

gchar *output_format_round_plain(RinksTournament *tournament, gint64 data, RinksRound *round_pf)
{
    RinksRound *round = round_pf != NULL ? round_pf : tournament_get_round(tournament, data);
    if (round == NULL)
        return NULL;
    GList *encounters = tournament_get_encounters(tournament, data, 0);
    if (encounters == NULL)
        return NULL;

    gchar *result = output_format_encounters_plain(tournament, encounters);

    g_list_free_full(encounters, (GDestroyNotify)encounter_free);

    if (round_pf == NULL) {
        round_free(round);
    }

    return result;
}

gchar *output_format_game_plain(RinksTournament *tournament, gint64 data, RinksGame *game_pf)
{
    RinksGame *game = game_pf != NULL ? game_pf : tournament_get_game(tournament, data);
    if (game == NULL)
        return NULL;
    GList *encounters = tournament_get_encounters(tournament, 0, data);
    if (encounters == NULL)
        return NULL;

    gchar *result = output_format_encounters_plain(tournament, encounters);

    g_list_free_full(encounters, (GDestroyNotify)encounter_free);

    if (game_pf == NULL) {
        game_free(game);
    }

    return result;
}

gchar *output_format_standings_plain(RinksTournament *tournament, RinksOutputType type, gint64 data, GList *teams_pf)
{
    gchar **standings = output_format_standings_raw(tournament, type, data, teams_pf);
    if (standings == NULL)
        return NULL;

    GString *str = g_string_new("");
    guint32 i = 0;
    while (standings[i] != NULL) {
        g_string_append_printf(str, "%.2s. %s\t%s\t%s\t%s\n", standings[i], standings[i+1],
                standings[i+2], standings[i+3], standings[i+4]);
        i += 5;
    }
    g_strfreev(standings);
    return g_string_free(str, FALSE);
} 

/* prefectched: if not NULL contains data which would be retrieved for this type */
gchar *output_format_plain(RinksTournament *tournament, RinksOutputType type, gint64 data, gpointer prefetched)
{
    switch (type) {
        case RinksOutputTypeRanking:
        case RinksOutputTypeRankingGroup:
            return output_format_standings_plain(tournament, type, data, prefetched);
        case RinksOutputTypeGameEncounter:
            return output_format_game_plain(tournament, data, (RinksGame *)prefetched);
        case RinksOutputTypeRoundEncounter:
            return output_format_round_plain(tournament, data, (RinksRound *)prefetched);
    }
    return NULL;
}

PangoLayout *output_print_prepare_layout(cairo_t *cr, gdouble width_in_mm, gint size, gboolean bold, PangoAlignment align)
{
    PangoLayout *layout;
    PangoFontDescription *desc;

    layout = pango_cairo_create_layout(cr);

    pango_layout_set_width(layout, width_in_mm * CAIRO_MM_PTS * PANGO_SCALE);

    gchar *font_string = g_strdup_printf("Helvetica %s%d", (bold ? "Bold " : ""), size);

    desc = pango_font_description_from_string(font_string);
    g_free(font_string);

/*    pango_layout_set_justify(layout, TRUE);*/
    pango_layout_set_alignment(layout, align);

    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    return layout;
}

gdouble output_print_line(cairo_t *cr, gdouble xoffset, gdouble yoffset, gdouble width,
                          gchar *text, gint size, gboolean bold, PangoAlignment align)
{
    cairo_identity_matrix(cr);
    cairo_translate(cr, xoffset*CAIRO_MM_PTS, yoffset*CAIRO_MM_PTS/PANGO_SCALE);

    PangoLayout *layout = output_print_prepare_layout(cr, width, size, bold, align);

    pango_layout_set_markup(layout, text, -1);

    int height;
    pango_layout_get_size(layout, NULL, &height);
    
    pango_cairo_update_layout(cr, layout);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);

    return (gdouble)height/CAIRO_MM_PTS;
}

gdouble output_print_heading(cairo_t *cr, gdouble xoffset, gdouble yoffset, gchar *heading, RinksTournament *tournament)
{
    return output_print_line(cr, xoffset, yoffset, 150.0, heading, 12, TRUE, PANGO_ALIGN_LEFT);
}

gdouble output_print_title(cairo_t *cr, gdouble xoffset, gdouble yoffset, RinksTournament *tournament)
{
    gdouble height = 0;

    /* print tournament title */
    gchar *title = tournament_get_property(tournament, "tournament.description");
/*    height += output_print_heading(cr, xoffset, yoffset, title, tournament);*/
    height += output_print_line(cr, xoffset, yoffset, 150.0, title, 18, TRUE, PANGO_ALIGN_LEFT);
    g_free(title);
    /* print current time */

    time_t curtime = time(NULL);
    struct tm *tm = localtime(&curtime);
    gchar buffer[512];

    strftime(buffer, 511, "Stand: %d.%m.%Y %H:%M Uhr", tm);
    height += output_print_line(cr, xoffset, yoffset + height, 150.0, buffer, 8, FALSE, PANGO_ALIGN_LEFT);

    return height;
}

gdouble output_print_standings(cairo_t *cr, gdouble xoffset, gdouble yoffset, RinksTournament *tournament)
{
    gchar **standings = output_format_standings_raw(tournament, RinksOutputTypeRanking, 0, NULL);
    if (standings == NULL)
        return 0.0;

    gdouble height = 0.0;
    gdouble curheight, maxheight;

    height += output_print_heading(cr, xoffset, yoffset, "Aktuelle Gesamtrangliste", tournament);

    maxheight = output_print_line(cr, xoffset, yoffset + height, 7.0, "Rg", 10, TRUE, PANGO_ALIGN_RIGHT);

    curheight = output_print_line(cr, xoffset + 12.0, yoffset + height, 110.0, "Team", 10, TRUE, PANGO_ALIGN_LEFT);
    if (curheight > maxheight)
        maxheight = curheight;
    curheight = output_print_line(cr, xoffset + 127.0, yoffset + height, 7.0, "P", 10, TRUE, PANGO_ALIGN_RIGHT);
    if (curheight > maxheight)
        maxheight = curheight;
    curheight = output_print_line(cr, xoffset + 135.0, yoffset + height, 7.0, "E", 10, TRUE, PANGO_ALIGN_RIGHT);
    if (curheight > maxheight)
        maxheight = curheight;
    curheight = output_print_line(cr, xoffset + 143.0, yoffset + height, 7.0, "S", 10, TRUE, PANGO_ALIGN_RIGHT);
    if (curheight > maxheight)
        maxheight = curheight;
    height += maxheight;

    guint32 i = 0;
    while (standings[i] != NULL) {
        /* pango_layout_set_tabs */
        maxheight = output_print_line(cr, xoffset, yoffset + height, 7.0, standings[i], 10, FALSE, PANGO_ALIGN_RIGHT);

        curheight = output_print_line(cr, xoffset + 12.0, yoffset + height, 110.0, standings[i + 1], 10, FALSE, PANGO_ALIGN_LEFT);
        if (curheight > maxheight)
            maxheight = curheight;
        curheight = output_print_line(cr, xoffset + 127.0, yoffset + height, 7.0, standings[i + 2], 10, FALSE, PANGO_ALIGN_RIGHT);
        if (curheight > maxheight)
            maxheight = curheight;
        curheight = output_print_line(cr, xoffset + 135.0, yoffset + height, 7.0, standings[i + 3], 10, FALSE, PANGO_ALIGN_RIGHT);
        if (curheight > maxheight)
            maxheight = curheight;
        curheight = output_print_line(cr, xoffset + 143.0, yoffset + height, 7.0, standings[i + 4], 10, FALSE, PANGO_ALIGN_RIGHT);
        if (curheight > maxheight)
            maxheight = curheight;
        height += maxheight;
        
        i += 5;
    }

    return height;
}

gdouble output_print_group_standings(cairo_t *cr, gdouble xoffset, gdouble offset, RinksTournament *tournament, gint32 group_id)
{
    return 0.0;
}

gdouble output_print_game_encounter(cairo_t *cr, gdouble xoffset, gdouble offset, RinksTournament *tournament, gint64 game_id)
{
    return 0.0;
}

gboolean output_print(RinksTournament *tournament, const gchar *filename)
{
    cairo_t *cr;
    cairo_surface_t *surface;

    surface = cairo_pdf_surface_create(filename, CAIRO_MM_PTS * 210.0, CAIRO_MM_PTS * 297.0);
    cr = cairo_create(surface);

    gdouble yoffset = 20.0;
    gdouble xoffset = 30.0;

    /* render tournament title */
    yoffset += output_print_title(cr, xoffset, yoffset, tournament);
    yoffset += 5.0;
    yoffset += output_print_standings(cr, xoffset, yoffset, tournament);
            /*cairo_show_page(cr);*/
    /* wenn in liste: render standings */
    /* wenn in liste: render group standings */
    /* wenn in liste: render game encounters */
    
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return TRUE;
}
