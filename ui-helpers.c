#include "ui-helpers.h"
#include <glib/gprintf.h>

GtkWidget *ui_helper_combo_widget_build(GtkTreeModel *model)
{
    g_printf("helper: widget build\n");
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkWidget *combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), renderer, "text", 0);

    return combo;
}

void ui_helper_clear_tree_model(GtkTreeModel *model)
{
    g_printf("helper: clear tree model\n");
    if (model == NULL)
        return;
    gtk_list_store_clear(GTK_LIST_STORE(model));
}

void ui_helper_build_combo_tree_model(GtkTreeModel **model, UiHelperModelType type, GList *entries)
{
    g_printf("helper: build tree model\n");
    g_return_if_fail(model != NULL);

    if (*model == NULL) {
        *model = GTK_TREE_MODEL(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT64));
    }

    GList *tmp;
    GtkTreeIter iter;
    gchar *text;

    g_printf("add empty argument\n");
    gtk_list_store_append(GTK_LIST_STORE(*model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(*model), &iter,
            0, "-- Auswahl --",
            1, 0,
            -1, -1); /* why the second -1? typo?? */

    g_printf("add entries\n");
    for (tmp = entries; tmp != NULL; tmp = g_list_next(tmp)) {
        gtk_list_store_append(GTK_LIST_STORE(*model), &iter);
        switch (type) {
            case UiHelperModelTypeEncounters:
                text = encounters_translate((RinksEncounter *)tmp->data);
                gtk_list_store_set(GTK_LIST_STORE(*model), &iter,
                        0, text,
                        1, ((RinksEncounter *)tmp->data)->id,
                        -1);
                g_free(text);
                break;
            case UiHelperModelTypeTeams:
                gtk_list_store_set(GTK_LIST_STORE(*model), &iter,
                        0, ((RinksTeam *)tmp->data)->name,
                        1, ((RinksTeam *)tmp->data)->id,
                        -1);
                break;
        }
    }
}

void ui_helper_combo_widget_set_selection(GtkWidget *widget, gint64 id)
{
    g_printf("helper: set selection\n");
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    if (model == NULL)
        return;

    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);

    gint64 entry_id;
    while (valid) {
        gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 1, &entry_id, -1);

        if (entry_id == id) {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget), &iter);
            return;
        }

        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
    }
}

gint64 ui_helper_combo_widget_get_selection(GtkWidget *widget)
{
    g_printf("helper: get selection\n");
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    if (model == NULL)
        return -1;

    if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
        return -1;

    gint64 id;
    gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 1, &id, -1);

    return id;
}
