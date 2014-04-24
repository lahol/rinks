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

gint64 db_add_team(gpointer db_handle, RinksTeam *team)
{
    g_return_val_if_fail(db_handle != NULL, -1);
    g_return_val_if_fail(team != NULL, -1);

    int rc;

    char *sql = sqlite3_mprintf("insert into teams (name,skip,group_id,points,ends,stones) values (%Q,%Q,%d,%d,%d,%d)",
            team->name, team->skip, team->group_id,
            team->points, team->ends, team->stones);

    rc = sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        return -1;

    return sqlite3_last_insert_rowid(db_handle);
}

/*gpointer *util_glist_to_array(GList *list)
{
    guint len = g_list_length(list);
    guint i;

    gpointer *array = g_malloc0(sizeof(gpointer) * (len + 1));
    for (i = 0; i < len; ++i, list = g_list_next(list))
        array[i] = list->data;

    return array;
}
*/
void db_update_team(gpointer db_handle, RinksTeam *team)
{
    g_return_if_fail(db_handle != NULL);
    g_return_if_fail(team != NULL);

    int rc;

    /*char *sql = sqlite3_mprintf("update teams set name=%Q, skip=%Q, group_id=%d, points=%d, ends=%d, stones=%d where id=%d",
            team->name, team->skip, team->group_id,
            team->points, team->ends, team->stones, team->id);*/

    char **keys = g_malloc0(sizeof(char *) * 7);
    int key_pos = 0;
    if (team->valid_keys & RinksTeamKeyName)
        keys[key_pos++] = sqlite3_mprintf("name=%Q", team->name);
    if (team->valid_keys & RinksTeamKeySkip)
        keys[key_pos++] = sqlite3_mprintf("skip=%Q", team->skip);
    if (team->valid_keys & RinksTeamKeyGroupId)
        keys[key_pos++] = sqlite3_mprintf("group_id=%d", team->group_id);
    if (team->valid_keys & RinksTeamKeyPoints)
        keys[key_pos++] = sqlite3_mprintf("points=%d", team->points);
    if (team->valid_keys & RinksTeamKeyEnds)
        keys[key_pos++] = sqlite3_mprintf("ends=%d", team->ends);
    if (team->valid_keys & RinksTeamKeyStones)
        keys[key_pos++] = sqlite3_mprintf("stones=%d", team->stones);

    if (key_pos == 0)
        return;

    char *set_entries = g_strjoinv(",", keys);

    char *sql = sqlite3_mprintf("update teams set %s where id=%d", set_entries, team->id);
    do {
        sqlite3_free(keys[--key_pos]);
    } while (key_pos);
    g_free(keys);
    g_free(set_entries);

    rc = sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        return;
}

GList *db_get_teams(gpointer db_handle)
{
    g_return_val_if_fail(db_handle != NULL, NULL);

    GList *result = NULL;
    int rc;
    sqlite3_stmt *stmt = NULL;
    RinksTeam *team;

    int col_count, col;
    const char *col_name;
    
    rc = sqlite3_prepare_v2(db_handle, "select * from teams order by id asc", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        goto out;

    col_count = sqlite3_column_count(stmt);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        team = g_malloc0(sizeof(RinksTeam));
        for (col = 0; col < col_count; ++col) {
            col_name = sqlite3_column_origin_name(stmt, col);
            if (g_strcmp0(col_name, "id") == 0) {
                team->id = sqlite3_column_int(stmt, col);
            }
            else if (g_strcmp0(col_name, "name") == 0) {
                team->name = g_strdup((gchar *)sqlite3_column_text(stmt, col));
            }
            else if (g_strcmp0(col_name, "skip") == 0) {
                team->skip = g_strdup((gchar *)sqlite3_column_text(stmt, col));
            }
            else if (g_strcmp0(col_name, "group_id") == 0) {
                team->group_id = sqlite3_column_int(stmt, col);
            }
            else if (g_strcmp0(col_name, "points") == 0) {
                team->points = sqlite3_column_int(stmt, col);
            }
            else if (g_strcmp0(col_name, "ends") == 0) {
                team->ends = sqlite3_column_int(stmt, col);
            }
            else if (g_strcmp0(col_name, "stones") == 0) {
                team->stones = sqlite3_column_int(stmt, col);
            }
        }

        result = g_list_append(result, team);
    }

out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return result;
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
