#include <glib/gprintf.h>
#include "ui.h"

void (*_menu_callback)(gchar *) = NULL;

void ui_set_action_callback(void (*callback)(gchar *action))
{
    _menu_callback = callback;
}

static gboolean _delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_main_quit();

    return FALSE;
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

GtkWidget *ui_create_sidebar(void)
{
    GtkWidget *list = gtk_tree_view_new();
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

    gtk_widget_show_all(scroll);

    return scroll;
}

GtkWidget *ui_create_main_window(void)
{
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    g_signal_connect(window, "delete-event", G_CALLBACK(_delete_event), NULL);

    gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);

    GtkWidget *menubar, *grid, *pane, *child;

    menubar = ui_create_main_menu();

    grid = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(grid), menubar, FALSE, FALSE, 0);

    pane = gtk_hpaned_new();

    child = ui_create_sidebar();
    gtk_paned_pack1(GTK_PANED(pane), child, FALSE, FALSE);

    child = ui_create_main_view();
    gtk_paned_pack2(GTK_PANED(pane), child, TRUE, TRUE);

    gtk_box_pack_end(GTK_BOX(grid), pane, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(window), grid);

    gtk_widget_show_all(window);

    return window;
}
