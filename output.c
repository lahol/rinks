#include "output.h"
#include <glib/gprintf.h>
#include <pango/pangocairo.h>
#include <cairo.h>
#include <cairo-pdf.h>
#include <time.h>

#define CAIRO_MM_PTS   (2.83)

typedef struct {
    RinksOutputType type;
    gint64 data;
} RinksOutputData;

struct OutputPagePartialLine {
    PangoLayout *layout;
    gdouble pos;
    gdouble height;
};

struct OutputPageContext {
    cairo_t *cr;
    cairo_surface_t *surf;
    gdouble yoffset;
    gdouble margin[4]; /* top bottom left right */
    gdouble pagesize[2];
    gdouble textwidth;
    gdouble textheight;
    gboolean in_line;
    GList *partial_line;
    gdouble line_height;
    gchar *footer;
    gint page_num;
    gdouble orphan_risk;
};

GList *output_data = NULL;

void output_print_footer(struct OutputPageContext *ctx);

void output_init(void)
{
    output_clear();
}

void output_clear(void)
{
    g_list_free_full(output_data, g_free);
    output_data = NULL;
}

gint output_types_compare(RinksOutputData *a, RinksOutputData *b)
{
    if (a->type < b->type)
        return -1;
    if (a->type > b->type)
        return 1;
    if (a->data < b->data)
        return -1;
    if (a->data > b->data)
        return 1;
    return 0;
}

void output_add(RinksOutputType type, gint64 data)
{
    RinksOutputData *entry = g_malloc0(sizeof(RinksOutputData));
    entry->type = type;
    entry->data = data;

    output_data = g_list_insert_sorted(output_data, entry, (GCompareFunc)output_types_compare);
}

gboolean output_page_context_init(struct OutputPageContext *ctx, const gchar *filename)
{
    ctx->surf = cairo_pdf_surface_create(filename, CAIRO_MM_PTS * 210.0, CAIRO_MM_PTS * 297.0);
    if (ctx->surf == NULL)
        return FALSE;

    ctx->cr = cairo_create(ctx->surf);

    ctx->margin[0] = ctx->margin[1] = 20.0 * CAIRO_MM_PTS;
    ctx->margin[2] = ctx->margin[3] = 30.0 * CAIRO_MM_PTS;

    ctx->yoffset = ctx->margin[0];
    ctx->textwidth = 210 * CAIRO_MM_PTS - ctx->margin[2] - ctx->margin[3];
    ctx->textheight = 297 * CAIRO_MM_PTS - ctx->margin[0] - ctx->margin[1];

    ctx->in_line = FALSE;
    ctx->partial_line = NULL;
    ctx->footer = NULL;
    ctx->page_num = 1;

    return TRUE;
}

void output_page_context_free_partial_line(struct OutputPagePartialLine *line)
{
    if (line == NULL)
        return;
    g_object_unref(line->layout);

    g_free(line);
}

void output_page_context_free(struct OutputPageContext *ctx)
{
    if (ctx->cr)
        cairo_destroy(ctx->cr);
    if (ctx->surf)
        cairo_surface_destroy(ctx->surf);
    g_free(ctx->footer);
    g_list_free_full(ctx->partial_line, (GDestroyNotify)output_page_context_free_partial_line);
}

void output_page_context_output(struct OutputPageContext *ctx, gboolean orphan_risk)
{
    GList *tmp;
    if (ctx->line_height + ctx->yoffset > ctx->textheight ||
        (orphan_risk && ctx->line_height + ctx->yoffset + 25.0 > ctx->textheight)) {
        output_print_footer(ctx);
        ++(ctx->page_num);
        cairo_show_page(ctx->cr);
        ctx->yoffset = ctx->margin[0];
    }

    for (tmp = g_list_last(ctx->partial_line); tmp != NULL; tmp = g_list_previous(tmp)) {
        cairo_identity_matrix(ctx->cr);
        cairo_translate(ctx->cr,
                ctx->margin[2] + ((struct OutputPagePartialLine *)tmp->data)->pos,
                ctx->yoffset);
        
        pango_cairo_update_layout(ctx->cr,
                ((struct OutputPagePartialLine *)tmp->data)->layout);
        pango_cairo_show_layout(ctx->cr,
                ((struct OutputPagePartialLine *)tmp->data)->layout);
    }

    ctx->yoffset += ctx->line_height;

    g_list_free_full(ctx->partial_line, (GDestroyNotify)output_page_context_free_partial_line);
    ctx->partial_line = NULL;
    ctx->line_height = 0.0;
}

void output_print_skip_space(struct OutputPageContext *ctx, gdouble space)
{
    if (space * CAIRO_MM_PTS + ctx->yoffset > ctx->textheight) {
        cairo_show_page(ctx->cr);
        ctx->yoffset = ctx->margin[0];
    }
    else {
        ctx->yoffset += space * CAIRO_MM_PTS;
    }
}

void output_page_context_partial_line_start(struct OutputPageContext *ctx)
{
    ctx->line_height = 0.0;
    ctx->in_line = TRUE;
}

void output_page_context_partial_line_end(struct OutputPageContext *ctx)
{
    output_page_context_output(ctx, ctx->orphan_risk);
    ctx->in_line = FALSE;
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
    gchar *result = NULL;

    if (encounters == NULL)
        goto out;

    result = output_format_encounters_plain(tournament, encounters);

    g_list_free_full(encounters, (GDestroyNotify)encounter_free);

out:
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

void output_print_line(struct OutputPageContext *ctx, gdouble xoffset, gdouble width,
                       gchar *text, gint size, gboolean bold, PangoAlignment align, gboolean orphan_risk)
{
    struct OutputPagePartialLine *line = g_malloc0(sizeof(struct OutputPagePartialLine));
    line->layout = output_print_prepare_layout(ctx->cr, width, size, bold, align);
    line->pos = xoffset * CAIRO_MM_PTS;

    pango_layout_set_markup(line->layout, text, -1);

    int height;
    pango_layout_get_size(line->layout, NULL, &height);
    gdouble this_line_height = (gdouble)height/((gdouble)PANGO_SCALE);
    if (this_line_height > ctx->line_height)
        ctx->line_height = this_line_height;

    ctx->partial_line = g_list_prepend(ctx->partial_line, line);
    ctx->orphan_risk = orphan_risk;

    if (!ctx->in_line) {
        output_page_context_output(ctx, orphan_risk);
    }
}

void output_set_footer_text(struct OutputPageContext *ctx, gchar *text)
{
    if (ctx->footer) {
        g_free(ctx->footer);
    }
    ctx->footer = g_strdup(text);
}

void output_print_footer(struct OutputPageContext *ctx)
{
    PangoLayout *layout;
    gchar *text;
    if (ctx->footer) {
        layout = output_print_prepare_layout(ctx->cr, ctx->textwidth/CAIRO_MM_PTS - 20.0, 8, FALSE, PANGO_ALIGN_LEFT);
        pango_layout_set_markup(layout, ctx->footer, -1);

        cairo_identity_matrix(ctx->cr);
        cairo_translate(ctx->cr,
                ctx->margin[2],
                297 * CAIRO_MM_PTS - ctx->margin[1] + 2.0 * CAIRO_MM_PTS);
        pango_cairo_update_layout(ctx->cr, layout);
        pango_cairo_show_layout(ctx->cr, layout);

        g_object_unref(layout);
    }

    text = g_strdup_printf("%d", ctx->page_num);

    layout = output_print_prepare_layout(ctx->cr, 18.0, 8, FALSE, PANGO_ALIGN_RIGHT);
    pango_layout_set_markup(layout, text, -1);

    cairo_identity_matrix(ctx->cr);
    cairo_translate(ctx->cr,
            ctx->margin[2] + ctx->textwidth - 18.0 * CAIRO_MM_PTS,
            297 * CAIRO_MM_PTS - ctx->margin[1] + 2.0 * CAIRO_MM_PTS);
    pango_cairo_update_layout(ctx->cr, layout);
    pango_cairo_show_layout(ctx->cr, layout);

    g_object_unref(layout);
    g_free(text);

}

void output_print_heading(struct OutputPageContext *ctx, gdouble xoffset, gchar *heading, RinksTournament *tournament)
{
    output_print_line(ctx, xoffset, 150.0, heading, 12, TRUE, PANGO_ALIGN_LEFT, TRUE);
}

void output_print_title(struct OutputPageContext *ctx, gdouble xoffset, RinksTournament *tournament)
{
    /* print tournament title */
    gchar *title = tournament_get_property(tournament, "tournament.description");
/*    height += output_print_heading(cr, xoffset, yoffset, title, tournament);*/
    if (title != NULL) {
        output_print_line(ctx, xoffset, 150.0, title, 18, TRUE, PANGO_ALIGN_LEFT, TRUE);
    }
    /* print current time */

    time_t curtime = time(NULL);
    struct tm *tm = localtime(&curtime);
    gchar buffer[512];

    strftime(buffer, 511, "Stand: %d.%m.%Y %H:%M Uhr", tm);
    output_print_line(ctx, xoffset, 150.0, buffer, 8, FALSE, PANGO_ALIGN_LEFT, FALSE);

    gchar *footer_text = g_strdup_printf("%s, %s", title ? title : "", buffer);
    output_set_footer_text(ctx, footer_text);
    g_free(footer_text);
    g_free(title);
}

void output_print_standings(struct OutputPageContext *ctx, gdouble xoffset, gchar **standings)
{
    if (standings == NULL)
        return;
    output_page_context_partial_line_start(ctx);
    output_print_line(ctx, xoffset, 7.0, "Rg", 10, TRUE, PANGO_ALIGN_RIGHT, TRUE);
    output_print_line(ctx, xoffset + 12.0, 110.0, "Team", 10, TRUE, PANGO_ALIGN_LEFT, TRUE);
    output_print_line(ctx, xoffset + 127.0, 7.0, "P", 10, TRUE, PANGO_ALIGN_RIGHT, TRUE);
    output_print_line(ctx, xoffset + 135.0, 7.0, "E", 10, TRUE, PANGO_ALIGN_RIGHT, TRUE);
    output_print_line(ctx, xoffset + 143.0, 7.0, "S", 10, TRUE, PANGO_ALIGN_RIGHT, TRUE);
    output_page_context_partial_line_end(ctx);

    guint32 i = 0;
    while (standings[i] != NULL) {
        /* pango_layout_set_tabs */
        output_page_context_partial_line_start(ctx);
        output_print_line(ctx, xoffset, 7.0, standings[i], 10, FALSE, PANGO_ALIGN_RIGHT, FALSE);
        output_print_line(ctx, xoffset + 12.0, 110.0, standings[i + 1], 10, FALSE, PANGO_ALIGN_LEFT, FALSE);
        output_print_line(ctx, xoffset + 127.0, 7.0, standings[i + 2], 10, FALSE, PANGO_ALIGN_RIGHT, FALSE);
        output_print_line(ctx, xoffset + 135.0, 7.0, standings[i + 3], 10, FALSE, PANGO_ALIGN_RIGHT, FALSE);
        output_print_line(ctx, xoffset + 143.0, 7.0, standings[i + 4], 10, FALSE, PANGO_ALIGN_RIGHT, FALSE);
        output_page_context_partial_line_end(ctx);
        
        i += 5;
    }

}

void output_print_standings_complete(struct OutputPageContext *ctx, gdouble xoffset, RinksTournament *tournament)
{
    gchar **standings = output_format_standings_raw(tournament, RinksOutputTypeRanking, 0, NULL);
    if (standings == NULL)
        return;

    output_print_heading(ctx, xoffset, "Aktuelle Gesamtrangliste", tournament);
    output_print_standings(ctx, xoffset, standings);

    g_strfreev(standings);
}

void output_print_group_standings(struct OutputPageContext *ctx, gdouble xoffset, RinksTournament *tournament, gint32 group_id)
{
    gchar **standings = output_format_standings_raw(tournament, RinksOutputTypeRankingGroup, group_id, NULL);
    if (standings == NULL)
        return;

    gchar *title = g_strdup_printf("Rangliste Gruppe %c", (gchar)(group_id - 1 + 'A'));
    output_print_heading(ctx, xoffset, title, tournament);
    g_free(title);
    output_print_standings(ctx, xoffset, standings);

    g_strfreev(standings);
}

void output_print_game_encounter(struct OutputPageContext *ctx, gdouble xoffset, RinksTournament *tournament, gint64 game_id)
{
    RinksGame *game = tournament_get_game(tournament, (gint32)game_id);
    if (game == NULL)
        return;
    GList *encounters = tournament_get_encounters(tournament, 0, (gint32)game_id);
    if (encounters == NULL) {
        game_free(game);
        return;
    }

    gchar *title = g_strdup_printf("Paarungen für %s", game->description);
    output_print_heading(ctx, xoffset, title, tournament);
    g_free(title);
    
    GList *tmp;
    gchar **line;
    for (tmp = encounters; tmp != NULL; tmp = g_list_next(tmp)) {
        line = output_format_game_encounter(tournament, ((RinksEncounter *)tmp->data));
        if (line != NULL) {
            output_page_context_partial_line_start(ctx);
            output_print_line(ctx, xoffset, 60.0, line[1], 10, FALSE, PANGO_ALIGN_LEFT, FALSE);
            output_print_line(ctx, xoffset + 62.0, 4, ":", 10, FALSE, PANGO_ALIGN_CENTER, FALSE);
            output_print_line(ctx, xoffset + 68.0, 60, line[2], 10, FALSE, PANGO_ALIGN_LEFT, FALSE);
            output_print_line(ctx, xoffset + 130.0, 20.0, line[0], 10, FALSE, PANGO_ALIGN_RIGHT, FALSE);
            output_page_context_partial_line_end(ctx);
        }
    }

    g_list_free_full(encounters, (GDestroyNotify)encounter_free);
    game_free(game);
}

gboolean output_print(RinksTournament *tournament, const gchar *filename)
{
    struct OutputPageContext page;
    if (!output_page_context_init(&page, filename))
        return FALSE;

    /* render tournament title */
    output_print_title(&page, 0.0, tournament);

    GList *tmp;
    for (tmp = output_data; tmp != NULL; tmp = g_list_next(tmp)) {
        switch (((RinksOutputData *)tmp->data)->type) {
            case RinksOutputTypeRanking:
                output_print_skip_space(&page, 5);
                output_print_standings_complete(&page, 0.0, tournament);
                break;
            case RinksOutputTypeRankingGroup:
                output_print_skip_space(&page, 5);
                output_print_group_standings(&page, 0.0, tournament, (gint32)((RinksOutputData *)tmp->data)->data);
                break;
            case RinksOutputTypeRoundEncounter:
                break;
            case RinksOutputTypeGameEncounter:
                output_print_skip_space(&page, 5);
                output_print_game_encounter(&page, 0.0, tournament, ((RinksOutputData *)tmp->data)->data);
                break;
        }

    }

    output_print_footer(&page);
    output_page_context_free(&page);
    
    return TRUE;
}
