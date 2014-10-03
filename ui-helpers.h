/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#pragma once

#include <gtk/gtk.h>
#include "encounters.h"
#include "teams.h"
#include "rounds.h"

typedef enum {
    UiHelperModelTypeEncounters,
    UiHelperModelTypeTeams,
    UiHelperModelTypeRound
} UiHelperModelType;

GtkWidget *ui_helper_combo_widget_build(GtkTreeModel *model);
void ui_helper_clear_tree_model(GtkTreeModel *model);
void ui_helper_build_combo_tree_model(GtkTreeModel **model, UiHelperModelType type, GList *data);
void ui_helper_combo_widget_set_selection(GtkWidget *widget, gint64 data);
gint64 ui_helper_combo_widget_get_selection(GtkWidget *widget);
