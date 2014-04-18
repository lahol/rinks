#include <glib/gprintf.h>
#include "ui.h"
#include "ui-dialog-settings.h"
#include "tournament.h"

GtkWidget *main_window = NULL;
GtkWidget *main_view = NULL;
GtkWidget *main_view_notebook = NULL;

GtkTreeStore *main_sidebar_tree = NULL;

void (*_menu_callback)(gchar *) = NULL;

enum {
    SIDEBAR_TYPE_SETTINGS,
    SIDEBAR_TYPE_TEAMS,
    SIDEBAR_TYPE_ROUNDS,
    SIDEBAR_TYPE_GAMES,
    SIDEBAR_TYPE_ENCOUNTERS,
    SIDEBAR_TYPE_RESULTS
};

enum {
    SIDEBAR_COLUMN_LABEL,
    SIDEBAR_COLUMN_PAGE,
    SIDEBAR_COLUMN_DATA,
    SIDEBAR_COLUMN_TYPE,
    SIDEBAR_N_COLUMNS
};

GtkWidget *ui_get_current_view(void)
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(main_view_notebook));
    GtkWidget *current = gtk_notebook_get_nth_page(GTK_NOTEBOOK(main_view_notebook), page);

    return current;

 /*    if (main_view == NULL)
        return NULL;

    GtkWidget *viewport = gtk_bin_get_child(GTK_BIN(main_view));
    if (viewport == NULL)
        return NULL;

    return gtk_bin_get_child(GTK_BIN(viewport));*/
    return NULL;
}

void ui_set_action_callback(void (*callback)(gchar *action))
{
    _menu_callback = callback;
}

static gboolean _delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_main_quit();

    return FALSE;
}

static void ui_sidebar_selection_changed(GtkTreeSelection *selection, gpointer data)
{
    g_printf("selection changed\n");

    GtkTreeIter iter;
    GtkTreeModel *model;
    gint pagenum;
    gpointer pagedata;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, SIDEBAR_COLUMN_PAGE, &pagenum,
                SIDEBAR_COLUMN_DATA, &pagedata, -1);
        g_printf("selection: %d: %p\n", pagenum, pagedata);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(main_view_notebook), pagenum);
    }
}

static void ui_apply_button_clicked(GtkButton *button, gpointer userdata)
{
    g_printf("Button clicked\n");
    GtkWidget *current = ui_get_current_view();

    if (current == NULL)
        return;

    UiDialogCallbacks *cb = g_object_get_data(G_OBJECT(current), "dialog-callbacks");

    if (cb && cb->apply_cb)
        cb->apply_cb(NULL);
}

static void ui_activate_menu_item(GtkMenuItem *menu_item, gpointer userdata)
{
    if (userdata && _menu_callback) {
        _menu_callback((gchar *)userdata);
    }
}

GtkWidget *ui_create_main_menu(void)
{
    GtkWidget *menu = gtk_menu_bar_new();

    GtkWidget *item;

    GtkWidget *submenu = gtk_menu_new();

    item = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ui_activate_menu_item), (gpointer)"actions.new");
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    gtk_widget_show(item);

    item = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ui_activate_menu_item), (gpointer)"actions.open");
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    gtk_widget_show(item);

    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), gtk_separator_menu_item_new());

    item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ui_activate_menu_item), (gpointer)"actions.quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    gtk_widget_show(item);

    item = gtk_menu_item_new_with_label("Aktionen");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    return menu;
}

GtkTreeStore *ui_create_sidebar_tree(void)
{
    main_sidebar_tree = gtk_tree_store_new(SIDEBAR_N_COLUMNS,
            G_TYPE_STRING,
            G_TYPE_INT,
            G_TYPE_POINTER,
            G_TYPE_INT);

    return main_sidebar_tree;
}

void ui_update_sidebar_tree(RinksTournament *tournament)
{
}

void ui_add_page(int type, const gchar *title, gpointer data)
{
    GtkWidget *page_widget = NULL;
    gint pagenum = -1;
    switch (type) {
        case SIDEBAR_TYPE_SETTINGS:
            page_widget = ui_dialog_settings_open(NULL);
            break;
        case SIDEBAR_TYPE_TEAMS:
        case SIDEBAR_TYPE_ROUNDS:
        case SIDEBAR_TYPE_GAMES:
        case SIDEBAR_TYPE_ENCOUNTERS:
        case SIDEBAR_TYPE_RESULTS:
            break;
    }

    if (page_widget == NULL)
        return;

    pagenum = gtk_notebook_append_page(GTK_NOTEBOOK(main_view_notebook), page_widget, NULL);
    if (pagenum == -1)
        return;

    GtkTreeIter iter;
    gtk_tree_store_append(main_sidebar_tree, &iter, NULL);

    gtk_tree_store_set(main_sidebar_tree, &iter,
            SIDEBAR_COLUMN_LABEL, title,
            SIDEBAR_COLUMN_PAGE, pagenum,
            SIDEBAR_COLUMN_DATA, data,
            SIDEBAR_COLUMN_TYPE, type,
            -1);
}

GtkWidget *ui_create_sidebar(void)
{
    GtkWidget *list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_create_sidebar_tree()));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Seite",
            renderer, "text", SIDEBAR_COLUMN_LABEL, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(selection), "changed",
            G_CALLBACK(ui_sidebar_selection_changed), NULL);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
            GTK_SHADOW_ETCHED_IN);

    gtk_widget_set_size_request(scroll, 150, 400);

    gtk_container_add(GTK_CONTAINER(scroll), list);
    gtk_widget_show_all(scroll);

    return scroll;
}

GtkWidget *ui_create_main_view(void)
{
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
            GTK_SHADOW_ETCHED_IN);

    gtk_widget_set_size_request(scroll, 150, 400);

    main_view_notebook = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(main_view_notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(main_view_notebook), FALSE);

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), main_view_notebook);

    gtk_widget_show_all(scroll);

    ui_add_page(SIDEBAR_TYPE_SETTINGS, "Einstellungen", NULL);

    return scroll;
}

GtkWidget *ui_create_main_window(void)
{
    GtkWidget *main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    g_signal_connect(main_window, "delete-event", G_CALLBACK(_delete_event), NULL);

    gtk_window_set_default_size(GTK_WINDOW(main_window), 640, 480);

    GtkWidget *menubar, *grid, *pane, *child, *vbox, *button;

    menubar = ui_create_main_menu();

    grid = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(grid), menubar, FALSE, FALSE, 0);

    pane = gtk_hpaned_new();

    child = ui_create_sidebar();
    gtk_paned_pack1(GTK_PANED(pane), child, FALSE, FALSE);

    main_view = ui_create_main_view();

    vbox = gtk_vbox_new(FALSE, 0);
    button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(ui_apply_button_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), main_view, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    gtk_paned_pack2(GTK_PANED(pane), vbox, TRUE, TRUE);

    gtk_box_pack_end(GTK_BOX(grid), pane, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(main_window), grid);

    gtk_widget_show_all(main_window);

    return main_window;
}

gchar *ui_get_filename(GtkFileChooserAction action)
{
    gchar *filename = NULL;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(action == GTK_FILE_CHOOSER_ACTION_OPEN ?
            "Turnier öffnen" : "Turnier anlegen",
            GTK_WINDOW(main_window), action,
            "Abbrechen", GTK_RESPONSE_CANCEL,
            "Auswählen", GTK_RESPONSE_ACCEPT,
            NULL);

    GtkFileFilter *filter;

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Turnier-Dateien");
    gtk_file_filter_add_pattern(filter, "*.rnk");
    gtk_file_filter_add_pattern(filter, "*.rinks");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Alle Dateien");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    if (action == GTK_FILE_CHOOSER_ACTION_SAVE) {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "Neues Turnier.rnk");
    }

    gint res = gtk_dialog_run(GTK_DIALOG(dialog));

    if (res == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }

    gtk_widget_destroy(dialog);

    return filename;
}

void ui_select_view(UiViewType type, gpointer data)
{
/*    GtkWidget *new_view = NULL;
    switch (type) {
        case VIEW_SETTINGS:
            new_view = ui_dialog_settings_open(data);
            break;
        case VIEW_TEAMS:
            break;
        default:
            break;
    }

    if (main_view != NULL) {
        GtkWidget *old_view = ui_get_current_view();
        if (old_view != NULL) {
            gtk_widget_hide(old_view);
            gtk_container_remove(GTK_CONTAINER(main_view), old_view);
        }
        if (new_view != NULL)
            gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(main_view), new_view);
    }*/
}

void ui_update_view(void)
{
    g_printf("update view\n");
    GtkWidget *current = ui_get_current_view();
    if (current == NULL)
        return;

    UiDialogCallbacks *cb = g_object_get_data(G_OBJECT(current),
            "dialog-callbacks");

    if (cb && cb->update_cb)
        cb->update_cb(NULL);
}

void ui_cleanup(void)
{
    gint npages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(main_view_notebook));
    gint i;
    GtkWidget *page;
    UiDialogCallbacks *cb = NULL;
    for (i = 0; i < npages; ++i) {
        page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(main_view_notebook), i);
        if (page == NULL)
            continue;
        cb = g_object_get_data(G_OBJECT(page), "dialog-callbacks");
        if (cb && cb->destroy_cb)
            cb->destroy_cb(NULL);
    }
}
