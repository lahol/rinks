#include "ui-dialog-settings.h"
#include "ui.h"
#include <glib/gprintf.h>

gpointer ui_dialog_settings_data = NULL;
GtkWidget *ui_dialog_settings = NULL;

void ui_dialog_settings_apply_cb(gpointer data)
{
    g_printf("ui-dialog-settings: apply\n");
}

GtkWidget *ui_dialog_settings_open(gpointer data)
{
    if (!GTK_IS_WIDGET(ui_dialog_settings)) {
        ui_dialog_settings = gtk_vbox_new(FALSE, 0);
        g_object_ref(G_OBJECT(ui_dialog_settings));

        g_object_set_data(G_OBJECT(ui_dialog_settings),
                "apply-callback", ui_dialog_settings_apply_cb);
   /* 
                "view-type", VIEW_SETTINGS,
                NULL);*/

        GtkWidget *label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), "<span size=\"xx-large\" weight=\"ultrabold\">Einstellungen</span>");

        gtk_box_pack_start(GTK_BOX(ui_dialog_settings), label, FALSE, FALSE, 0);
    }

    ui_dialog_settings_data = data;

    /* fill with data */

    gtk_widget_show_all(ui_dialog_settings);

    return ui_dialog_settings;
}
