#include "db.h"
#include <sqlite3.h>
#include <glib/gprintf.h>

sqlite3 *db_database = NULL;

gint db_init_database(gchar *path)
{
    gint rc;
    char *sql;
    char *sql_err = NULL;

    rc = sqlite3_open(path, &db_database);
    if (rc != 0)
        goto out;

#define STMT_EXEC(stmt) do {\
    sql = stmt;\
    rc = sqlite3_exec(db_database, sql, NULL, NULL, &sql_err);\
    if (rc != SQLITE_OK)\
        goto out;\
} while (0)

    STMT_EXEC("create table if not exists settings(key varchar,\
        value varchar,\
        default_value varchar)");

    STMT_EXEC("create table if not exists teams(id integer primary key,\
        name varchar,\
        skip varchar,\
        group_id integer,\
        points integer,\
        ends integer,\
        stones integer)");

    STMT_EXEC("create table if not exists rounds(id integer primary key,\
        type varchar,\
        description varchar,\
        sequence int)");

    STMT_EXEC("create table if not exists games(id integer primary key,\
        description varchar,\
        sequence integer,\
        closed integer)");

    STMT_EXEC("create table if not exists encounters(id integer primary key,\
        team1 integer,\
        team2 integer,\
        round integer,\
        game integer,\
        rink integer)");

    STMT_EXEC("create table if not exists results(id integer primary key,\
        team integer,\
        encounter integer,\
        points integer,\
        ends integer,\
        stones integer)");

#undef STMT_EXEC

    return 0;

out:
    g_printf("error encountered: %s\n", sql_err ? sql_err : "(no error message)");
    if (sql_err)
    sqlite3_free(sql_err);
    db_cleanup();
    return 1;
}

gint db_open_database(gchar *path)
{
    gint rc;

    rc = sqlite3_open(path, &db_database);
    if (rc != 0)
        goto out;
    return 0;
out:
    db_cleanup();
    return 1;
}

void db_cleanup(void)
{
    g_printf("db_cleanup\n");
    if (db_database != NULL) {
        sqlite3_close(db_database);
        db_database = NULL;
    }
}

void db_add_team(RinksTeam *team)
{
}

GList *db_get_teams(void)
{
    return NULL;
}
