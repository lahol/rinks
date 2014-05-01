#include <glib/gprintf.h>

#include "ui-dialog-round-overview.h"
#include "ui.h"

#include "tournament.h"
#include "encounters.h"
#include "teams.h"
#include "application.h"

GtkWidget *ui_dialog_round_overview = NULL;
GtkWidget *ui_dialog_round_overview_text = NULL;

void ui_dialog_round_overview_activate_cb(gpointer data)
{
    g_printf("dialog-round-overview: activate: %p\n", data);
}

GtkWidget *ui_dialog_round_overview_open(gpointer data)
{
    static UiDialogCallbacks callbacks = {
        .activated_cb = ui_dialog_round_overview_activate_cb
    };

    if (!GTK_IS_WIDGET(ui_dialog_round_overview)) {
        ui_dialog_round_overview = ui_dialog_page_new("Rundenüberblick", &callbacks);

        ui_dialog_round_overview_text = gtk_text_view_new();
        gtk_box_pack_start(GTK_BOX(ui_dialog_round_overview),
                ui_dialog_round_overview_text, FALSE, FALSE, 3);
    }

    gtk_widget_show_all(ui_dialog_round_overview);

    return ui_dialog_round_overview;
}
