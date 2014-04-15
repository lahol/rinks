#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include "ui.h"

GtkWidget *main_window = NULL;
void menu_callback(gchar *action);

void init(void)
{
    main_window = ui_create_main_window();
    ui_set_action_callback(menu_callback);
}

void cleanup(void)
{
}

void menu_callback(gchar *action)
{
    if (g_strcmp0(action, "actions.quit") == 0) {
        gtk_main_quit();
    }
    else if (g_strcmp0(action, "actions.new") == 0) {
        g_printf("New Tournament\n");
    }
    else if (g_strcmp0(action, "actions.open") == 0) {
        g_printf("Open Tournament\n");
    }
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    init();

    gtk_main();

    cleanup();

    return 0;
}
