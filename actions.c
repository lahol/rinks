#include "actions.h"
#include "db.h"
#include <glib/gprintf.h>

void action_new_tournament(void)
{
    gint rc;

    rc = db_init_database("test.rinks");

    if (rc != 0)
        g_printf("init db error\n");
}

void action_open_tournament(void)
{
}
