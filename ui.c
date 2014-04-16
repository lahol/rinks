#include <glib/gprintf.h>
#include "ui.h"
#include "ui-dialog-settings.h"

GtkWidget *main_window = NULL;
GtkWidget *main_view = NULL;

void (*_menu_callback)(gchar *) = NULL;

GtkWidget *ui_get_current_view(void)
{
    if (main_view == NULL)
        return NULL;

    GtkWidget *viewport = gtk_bin_get_child(GTK_BIN(main_view));
    if (viewport == NULL)
        return NULL;

    return gtk_bin_get_child(GTK_BIN(viewport));
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

static void ui_apply_button_clicked(GtkButton *button, gpointer userdata)
{
    g_printf("Button clicked\n");
    GtkWidget *current = ui_get_current_view();

    if (current == NULL)
        return;

    void (*apply_cb)(gpointer) = g_object_get_data(G_OBJECT(current), "apply-callback");

    if (apply_cb)
        apply_cb(NULL);
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
    GtkWidget *new_view = NULL;
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
    }
}
