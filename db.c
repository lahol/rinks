#include "db.h"
#include <sqlite3.h>
#include <glib/gprintf.h>

gpointer db_init_database(gchar *path, gboolean clear)
{
    gint rc;
    char *sql;
    char *sql_err = NULL;
    sqlite3 *db_handle = NULL;

    rc = sqlite3_open(path, &db_handle);
    if (rc != 0)
        goto out;

#define CREATE_TABLE(table, schema) do {\
    if (clear)\
        sql = "drop table if exists " table "; create table if not exists " table schema;\
    else\
        sql = "create table if not exists " table schema;\
    rc = sqlite3_exec(db_handle, sql, NULL, NULL, &sql_err);\
    if (rc != SQLITE_OK)\
        goto out;\
} while (0)

    CREATE_TABLE("settings", "(key varchar,\
        value varchar,\
        default_value varchar)");

    CREATE_TABLE("teams", "(id integer primary key,\
        name varchar,\
        skip varchar,\
        group_id integer,\
        points integer,\
        ends integer,\
        stones integer)");

    CREATE_TABLE("rounds", "(id integer primary key,\
        type varchar,\
        description varchar,\
        sequence int)");

    CREATE_TABLE("games", "(id integer primary key,\
        description varchar,\
        sequence integer,\
        closed integer)");

    CREATE_TABLE("encounters", "(id integer primary key,\
        team1 integer,\
        team2 integer,\
        round integer,\
        game integer,\
        rink integer)");

    CREATE_TABLE("results", "(id integer primary key,\
        team integer,\
        encounter integer,\
        points integer,\
        ends integer,\
        stones integer)");

#undef STMT_EXEC

    return db_handle;

out:
    g_printf("error encountered: %s\n", sql_err ? sql_err : "(no error message)");
    if (sql_err)
    sqlite3_free(sql_err);
    db_close_database(db_handle);
    return NULL;
}

gpointer db_open_database(gchar *path, gboolean clear)
{
    gpointer db_handle = db_init_database(path, clear);

    return db_handle;
}

void db_close_database(gpointer db_handle)
{
    g_printf("db_cleanup\n");
    if (db_handle != NULL) {
        sqlite3_close(db_handle);
    }
}

void db_add_team(gpointer db_handle, RinksTeam *team)
{
}

GList *db_get_teams(gpointer db_handle)
{
    return NULL;
}

void db_set_property(gpointer db_handle, const gchar *key, const gchar *value)
{
    g_return_if_fail(db_handle != NULL);
    g_return_if_fail(key != NULL && key[0] != '\0');

    int rc;

    char *sql = sqlite3_mprintf("update settings set value=%Q where key=%Q",
            value, key);
    rc = sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        return;

    if (sqlite3_changes(db_handle) == 0) {
        sql = sqlite3_mprintf("insert into settings (key,value) values (%Q,%Q)", key, value);
        /*rc =*/ sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
        sqlite3_free(sql);
    }
}

gchar *db_get_property(gpointer db_handle, const gchar *key)
{
    g_return_val_if_fail(db_handle != NULL, NULL);
    g_return_val_if_fail(key != NULL && key[0] != '\0', NULL);

    int rc;
    sqlite3_stmt *stmt = NULL;
    gchar *result = NULL;

    char *sql = sqlite3_mprintf("select value from settings where key = %Q", key);

    rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        goto out;

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
        goto out;

    result = g_strdup((gchar *)sqlite3_column_text(stmt, 0));

out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    if (sql != NULL)
        sqlite3_free(sql);
    return result;
}
