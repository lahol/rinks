/*
 * rinks -- tournament planner for curling
 * (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#include "db.h"
#include <sqlite3.h>
#include <glib/gprintf.h>
#include <glib-object.h>

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
        range_start int,\
        range_end int,\
        flags int,\
        sequence int)");

    CREATE_TABLE("games", "(id integer primary key,\
        description varchar,\
        sequence integer,\
        closed integer)");
    
    /* abstract team: group:position -> fixed for given round */
    CREATE_TABLE("encounters", "(id integer primary key,\
        abstract_team1 varchar,\
        abstract_team2 varchar,\
        real_team1 integer,\
        real_team2 integer,\
        round integer,\
        game integer,\
        rink integer)");

    CREATE_TABLE("results", "(id integer primary key,\
        team integer,\
        encounter integer,\
        points integer,\
        ends integer,\
        stones integer)");

    CREATE_TABLE("overrides", "(id integer primary key,\
        encounter integer,\
        team1 integer,\
        team2 integer)");

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

gboolean db_sql_extract_value(sqlite3_stmt *stmt, int colnum, const gchar *name, GType type, gpointer target)
{
    const gchar *col_name = sqlite3_column_origin_name(stmt, colnum);
    if (g_strcmp0(col_name, name) != 0)
        return FALSE;

    if (target == NULL)
        return TRUE;

    switch (type) {
        case G_TYPE_INT:
            *((gint *)target) = sqlite3_column_int(stmt, colnum);
            break;
        case G_TYPE_INT64:
            *((gint64 *)target) = sqlite3_column_int64(stmt, colnum);
            break;
        case G_TYPE_UINT:
            *((guint *)target) = (guint)sqlite3_column_int(stmt, colnum);
            break;
        case G_TYPE_STRING:
            *((gchar **)target) = g_strdup((gchar *)sqlite3_column_text(stmt, colnum));
            break;
        default:
            return FALSE;
    }

    return TRUE;
}

GList *db_get_teams(gpointer db_handle)
{
    g_return_val_if_fail(db_handle != NULL, NULL);

    GList *result = NULL;
    int rc;
    sqlite3_stmt *stmt = NULL;
    RinksTeam *team;

    int col_count, col;
    
    rc = sqlite3_prepare_v2(db_handle, "select * from teams order by id desc", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        goto out;

    col_count = sqlite3_column_count(stmt);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        team = g_malloc0(sizeof(RinksTeam));
        for (col = 0; col < col_count; ++col) {
#define DBGETVAL(name, type, val) if (db_sql_extract_value(stmt, col, (name), (type), &(val))) continue
            DBGETVAL("id", G_TYPE_INT64, team->id);
            DBGETVAL("name", G_TYPE_STRING, team->name);
            DBGETVAL("skip", G_TYPE_STRING, team->skip);
            DBGETVAL("group_id", G_TYPE_INT, team->group_id);
            DBGETVAL("points", G_TYPE_INT, team->points);
            DBGETVAL("ends", G_TYPE_INT, team->ends);
            DBGETVAL("stones", G_TYPE_INT, team->stones);
#undef DBGETVAL
        }

        result = g_list_prepend(result, team);
    }

out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return result;
}

RinksTeam *db_get_team(gpointer db_handle, gint64 team_id)
{
    g_return_val_if_fail(db_handle != NULL, NULL);

    int rc;
    sqlite3_stmt *stmt = NULL;
    RinksTeam *team = NULL;

    int col_count, col;

#ifdef WIN32
    gchar *sql = sqlite3_mprintf("select * from teams where id=%lld", team_id);
#else
    gchar *sql = sqlite3_mprintf("select * from teams where id=%" G_GINT64_FORMAT, team_id);
#endif
    rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        goto out;

    col_count = sqlite3_column_count(stmt);

    if ((rc = sqlite3_step(stmt)) != SQLITE_ROW)
        goto out;
    team = g_malloc0(sizeof(RinksTeam));
    for (col = 0; col < col_count; ++col) {
#define DBGETVAL(name, type, val) if (db_sql_extract_value(stmt, col, (name), (type), &(val))) continue
            DBGETVAL("id", G_TYPE_INT64, team->id);
            DBGETVAL("name", G_TYPE_STRING, team->name);
            DBGETVAL("skip", G_TYPE_STRING, team->skip);
            DBGETVAL("group_id", G_TYPE_INT, team->group_id);
            DBGETVAL("points", G_TYPE_INT, team->points);
            DBGETVAL("ends", G_TYPE_INT, team->ends);
            DBGETVAL("stones", G_TYPE_INT, team->stones);
#undef DBGETVAL
    }

out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return team;
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

gint db_get_team_count(gpointer db_handle)
{
    g_return_val_if_fail(db_handle != NULL, 0);

    int rc;
    sqlite3_stmt *stmt = NULL;
    gint count = 0;

    rc = sqlite3_prepare_v2(db_handle, "select count(*) from teams", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        goto out;

    if ((rc = sqlite3_step(stmt)) != SQLITE_ROW)
        goto out;

    count = sqlite3_column_int(stmt, 0);

out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return count;
}

GList *db_get_rounds(gpointer db_handle)
{
    g_return_val_if_fail(db_handle != NULL, NULL);

    GList *result = NULL;
    int rc;
    sqlite3_stmt *stmt = NULL;
    RinksRound *round;

    int col_count, col;

    /* TODO: maybe order by sequence -> need to write sequence */
    rc = sqlite3_prepare_v2(db_handle, "select * from rounds order by id desc", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        goto out;

    col_count = sqlite3_column_count(stmt);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        round = g_malloc0(sizeof(RinksRound));
        for (col = 0; col < col_count; ++col) {
#define DBGETVAL(name, type, val) if (db_sql_extract_value(stmt, col, (name), (type), &(val))) continue
            DBGETVAL("id", G_TYPE_INT64, round->id);
            DBGETVAL("type", G_TYPE_INT, round->type);
            DBGETVAL("description", G_TYPE_STRING, round->description);
            DBGETVAL("range_start", G_TYPE_INT, round->range_start);
            DBGETVAL("range_end", G_TYPE_INT, round->range_end);
            DBGETVAL("flags", G_TYPE_UINT, round->flags);
#undef DBGETVAL
        }

        g_printf("get rounds: %" G_GINT64_FORMAT " flags: 0x%x\n", round->id, round->flags);
        result = g_list_prepend(result, round);
    }
out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return result;
}

RinksRound *db_get_round(gpointer db_handle, gint64 round_id)
{
    g_return_val_if_fail(db_handle != NULL, NULL);

    int rc;
    sqlite3_stmt *stmt = NULL;
    RinksRound *round = NULL;

    int col_count, col;

#ifdef WIN32
    gchar *sql = sqlite3_mprintf("select * from rounds where id=%lld", round_id);
#else
    gchar *sql = sqlite3_mprintf("select * from rounds where id=%" G_GINT64_FORMAT, round_id);
#endif
    rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        goto out;

    col_count = sqlite3_column_count(stmt);

    if ((rc = sqlite3_step(stmt)) != SQLITE_ROW)
        goto out;
    round = g_malloc0(sizeof(RinksRound));
    for (col = 0; col < col_count; ++col) {
#define DBGETVAL(name, type, val) if (db_sql_extract_value(stmt, col, (name), (type), &(val))) continue
            DBGETVAL("id", G_TYPE_INT64, round->id);
            DBGETVAL("type", G_TYPE_INT, round->type);
            DBGETVAL("description", G_TYPE_STRING, round->description);
            DBGETVAL("range_start", G_TYPE_INT, round->range_start);
            DBGETVAL("range_end", G_TYPE_INT, round->range_end);
            DBGETVAL("flags", G_TYPE_UINT, round->flags);
#undef DBGETVAL
    }

out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return round;
}

gint64 db_add_round(gpointer db_handle, RinksRound *round)
{
    g_return_val_if_fail(db_handle != NULL, -1);
    g_return_val_if_fail(round != NULL, -1);

    int rc;

    g_printf("add round flags: 0x%x\n", round->flags);

    gchar *sql = sqlite3_mprintf("insert into rounds (type,description,range_start,range_end,flags) values (%d,%Q,%d,%d,%u)",
            round->type, round->description,
            round->range_start, round->range_end,
            round->flags);

    rc = sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        return -1;

    return sqlite3_last_insert_rowid(db_handle);
}

void db_update_round(gpointer db_handle, RinksRound *round)
{
    g_return_if_fail(db_handle != NULL);
    g_return_if_fail(round != NULL);

    int rc;

#ifdef WIN32
    gchar *sql = sqlite3_mprintf("update rounds set type=%d, description=%Q, range_start=%d,\
range_end=%d where id=%lld",
            round->type, round->description, round->range_start,
            round->range_end, round->id);
#else
    gchar *sql = sqlite3_mprintf("update rounds set type=%d, description=%Q, range_start=%d,\
range_end=%d where id=%" G_GINT64_FORMAT,
            round->type, round->description, round->range_start,
            round->range_end, round->id);
#endif

    rc = sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        return;
}

void db_round_update_flag(gpointer db_handle, gint64 round_id, guint flag, gboolean set)
{
    RinksRound *round = db_get_round(db_handle, round_id);

    if (round == NULL)
        return;
    
    if (set)
        round->flags |= flag;
    else
        round->flags &= ~flag;

    g_printf("db update flags: 0x%x\n", round->flags);

#ifdef WIN32
    gchar *sql = sqlite3_mprintf("update rounds set flags=%u where id=%lld",
            round->flags, round_id);
#else
    gchar *sql = sqlite3_mprintf("update rounds set flags=%u where id=%" G_GINT64_FORMAT,
            round->flags, round_id);
#endif
    gchar *err = NULL;
    int rc;
    rc = sqlite3_exec(db_handle, sql, NULL, NULL, &err);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        g_printf("db error: %s\n", err);
        sqlite3_free(err);
    }

    round_free(round);
}

void db_round_set_flag(gpointer db_handle, gint64 round_id, guint flag)
{
    db_round_update_flag(db_handle, round_id, flag, TRUE);
}

void db_round_unset_flag(gpointer db_handle, gint64 round_id, guint flag)
{
    db_round_update_flag(db_handle, round_id, flag, FALSE);
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

gint64 db_add_encounter(gpointer db_handle, gint64 round_id,
                        const gchar *abstr_team1, const gchar *abstr_team2)
{
    g_return_val_if_fail(db_handle != NULL, -1);

    int rc;

#ifdef WIN32
    char *sql = sqlite3_mprintf("insert into encounters (round,abstract_team1,abstract_team2,game,rink) values (%lld\
,%Q,%Q,-1,-1)",
            round_id, abstr_team1, abstr_team2);
#else
    char *sql = sqlite3_mprintf("insert into encounters (round,abstract_team1,abstract_team2,game,rink) values (%"\
G_GINT64_FORMAT ",%Q,%Q,-1,-1)",
            round_id, abstr_team1, abstr_team2);
#endif

    gchar *err = NULL;
    rc = sqlite3_exec(db_handle, sql, NULL, NULL, &err);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        g_printf("db error: %s\n", err);
        sqlite3_free(err);
        return -1;
    }

    return sqlite3_last_insert_rowid(db_handle);
}

gint64 db_add_encounter_full(gpointer db_handle, gint64 round_id,
                             const gchar *abstr_team1, const gchar *abstr_team2,
                             gint64 real_team1, gint64 real_team2)
{
    g_return_val_if_fail(db_handle != NULL, -1);

    int rc;

#ifdef WIN32
    char *sql = sqlite3_mprintf("insert into encounters (round,abstract_team1,abstract_team2,real_team1,real_team2,game,rink) values (%lld\
,%Q,%Q,%lld,%lld,-1,-1)",
            round_id, abstr_team1, abstr_team2, real_team1, real_team2);
#else
    char *sql = sqlite3_mprintf("insert into encounters (round,abstract_team1,abstract_team2,real_team1,real_team2,game,rink) values (%"\
G_GINT64_FORMAT ",%Q,%Q,%" G_GINT64_FORMAT ",%" G_GINT64_FORMAT ",-1,-1)",
            round_id, abstr_team1, abstr_team2, real_team1, real_team2);
#endif

    gchar *err = NULL;
    rc = sqlite3_exec(db_handle, sql, NULL, NULL, &err);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        g_printf("db error: %s\n", err);
        sqlite3_free(err);
        return -1;
    }

    return sqlite3_last_insert_rowid(db_handle);
}

void db_update_encounter(gpointer db_handle, RinksEncounter *encounter)
{
    g_return_if_fail(db_handle != NULL);
    g_return_if_fail(encounter != NULL);

    int rc;

#ifdef WIN32
    gchar *sql = sqlite3_mprintf("update encounters set abstract_team1=%Q, abstract_team2=%Q, real_team1=%lld,\
 real_team2=%lld, round=%lld, game=%lld, rink=%d where id=%lld", encounter->abstract_team1, encounter->abstract_team2,
            encounter->real_team1, encounter->real_team2, encounter->round, encounter->game,
            encounter->rink, encounter->id);
#else
    gchar *sql = sqlite3_mprintf("update encounters set abstract_team1=%Q, abstract_team2=%Q, real_team1=%"\
G_GINT64_FORMAT ", real_team2=%" G_GINT64_FORMAT ", round=%" G_GINT64_FORMAT ", game=%" G_GINT64_FORMAT\
", rink=%d where id=%" G_GINT64_FORMAT, encounter->abstract_team1, encounter->abstract_team2,
            encounter->real_team1, encounter->real_team2, encounter->round, encounter->game,
            encounter->rink, encounter->id);
#endif

    gchar *sql_err = NULL;
    rc = sqlite3_exec(db_handle, sql, NULL, NULL, &sql_err);
    sqlite3_free(sql);

    if (sql_err) {
        g_printf("sql err: %s\n", sql_err);
        sqlite3_free(sql_err);
    }
    if (rc != SQLITE_OK)
        return;
}

GList *db_get_encounters(gpointer db_handle, gint64 round_id, gint64 game_id)
{
    g_return_val_if_fail(db_handle != NULL, NULL);

    GList *result = NULL;
    int rc;
    sqlite3_stmt *stmt = NULL;
    RinksEncounter *encounter;

    int col_count, col;

    char *sql;
   
    if (round_id > 0 && game_id > 0) {
#ifdef WIN32
        sql = sqlite3_mprintf("select * from encounters where round=%lld and game=%lld order by id desc",
                                round_id, game_id);
#else
        sql = sqlite3_mprintf("select * from encounters where round=%" G_GINT64_FORMAT " and game=%" G_GINT64_FORMAT " order by id desc",
                                round_id, game_id);
#endif
    }
    else if (round_id > 0) {
#ifdef WIN32
        sql = sqlite3_mprintf("select * from encounters where round=%lld order by id desc",
                round_id);
#else
        sql = sqlite3_mprintf("select * from encounters where round=%" G_GINT64_FORMAT " order by id desc",
                round_id);
#endif
    }
    else if (game_id > 0) {
#ifdef WIN32
        sql = sqlite3_mprintf("select * from encounters where game=%lld order by id desc",
                game_id);
#else
        sql = sqlite3_mprintf("select * from encounters where game=%" G_GINT64_FORMAT " order by id desc",
                game_id);
#endif
    }
    else
        sql = sqlite3_mprintf("select * from encounters order by id desc");

    rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        goto out;
    
    col_count = sqlite3_column_count(stmt);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        encounter = g_malloc0(sizeof(RinksEncounter));
        for (col = 0; col < col_count; ++col) {
#define DBGETVAL(name, type, val) if (db_sql_extract_value(stmt, col, (name), (type), &(val))) continue
            DBGETVAL("id", G_TYPE_INT64, encounter->id);
            DBGETVAL("abstract_team1", G_TYPE_STRING, encounter->abstract_team1);
            DBGETVAL("abstract_team2", G_TYPE_STRING, encounter->abstract_team2);
            DBGETVAL("real_team1", G_TYPE_INT64, encounter->real_team1);
            DBGETVAL("real_team2", G_TYPE_INT64, encounter->real_team2);
            DBGETVAL("round", G_TYPE_INT64, encounter->round);
            DBGETVAL("game", G_TYPE_INT64, encounter->game);
            DBGETVAL("rink", G_TYPE_INT, encounter->rink);
#undef DBGETVAL
        }

        result = g_list_prepend(result, encounter);
    }
out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return result;
}

RinksEncounter *db_get_encounter(gpointer db_handle, gint64 encounter_id)
{
    g_return_val_if_fail(db_handle != NULL, NULL);

    if (encounter_id <= 0)
        return NULL;

    int rc;
    sqlite3_stmt *stmt = NULL;
    RinksEncounter *encounter = NULL;

    int col_count, col;

#ifdef WIN32
    char *sql = sqlite3_mprintf("select * from encounters where id=%lld", encounter_id);
#else
    char *sql = sqlite3_mprintf("select * from encounters where id=%" G_GINT64_FORMAT, encounter_id);
#endif

    rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        goto out;

    col_count = sqlite3_column_count(stmt);

    if ((rc = sqlite3_step(stmt)) != SQLITE_ROW)
        goto out;

    encounter = g_malloc0(sizeof(RinksEncounter));
    for (col = 0; col < col_count; ++col) {
#define DBGETVAL(name, type, val) if (db_sql_extract_value(stmt, col, (name), (type), &(val))) continue
        DBGETVAL("id", G_TYPE_INT64, encounter->id);
        DBGETVAL("abstract_team1", G_TYPE_STRING, encounter->abstract_team1);
        DBGETVAL("abstract_team2", G_TYPE_STRING, encounter->abstract_team2);
        DBGETVAL("real_team1", G_TYPE_INT64, encounter->real_team1);
        DBGETVAL("real_team2", G_TYPE_INT64, encounter->real_team2);
        DBGETVAL("round", G_TYPE_INT64, encounter->round);
        DBGETVAL("game", G_TYPE_INT64, encounter->game);
        DBGETVAL("rink", G_TYPE_INT, encounter->rink);
#undef DBGETVAL
    }

out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return encounter;
}

gboolean db_existed_encounter_before(gpointer db_handle, gint64 round_id, gint64 team1, gint64 team2)
{
    g_return_val_if_fail(db_handle != NULL, FALSE);

    int rc;
    sqlite3_stmt *stmt = NULL;
    gint count = 0;

#ifdef WIN32
    char *sql = sqlite3_mprintf("select count(*) from encounters where (real_team1=%lld and real_team2=%lld)\
or (real_team1=%lld and real_team2=%lld) and round < %lld",
            team1, team2, team2, team1, round_id);
#else
    char *sql = sqlite3_mprintf("select count(*) from encounters where (real_team1=%" G_GINT64_FORMAT " and real_team2=%"\
G_GINT64_FORMAT ") or (real_team1=%" G_GINT64_FORMAT " and real_team2=%" G_GINT64_FORMAT ") and round < %" G_GINT64_FORMAT,
            team1, team2, team2, team1, round_id);
#endif

    rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        goto out;

    if ((rc = sqlite3_step(stmt)) != SQLITE_ROW)
        goto out;

    count = sqlite3_column_int(stmt, 0);

out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    if (sql != NULL)
        sqlite3_free(sql);

    return (count > 0 ? TRUE : FALSE);
}

void db_encounter_set_game(gpointer db_handle, gint64 encounter_id, gint64 game_id)
{
    g_return_if_fail(db_handle != NULL);

    int rc;
    if (encounter_id <= 0)
        return;
#ifdef WIN32
    char *sql = sqlite3_mprintf("update encounters set game=%lld where id=%lld",
            game_id, encounter_id);
#else
    char *sql = sqlite3_mprintf("update encounters set game=%" G_GINT64_FORMAT " where id=%" G_GINT64_FORMAT,
            game_id, encounter_id);
#endif

    rc = sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
    sqlite3_free(sql);
}

gint64 db_add_game(gpointer db_handle, RinksGame *game)
{
    g_return_val_if_fail(db_handle != NULL, -1);
    g_return_val_if_fail(game != NULL, -1);

    int rc;

    char *sql = sqlite3_mprintf("insert into games (description,sequence,closed) values (%Q,%d,%d)",
            game->description, game->sequence, game->closed);

    rc = sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        return -1;

    return sqlite3_last_insert_rowid(db_handle);
}

GList *db_get_games(gpointer db_handle)
{
    g_return_val_if_fail(db_handle != NULL, NULL);

    GList *result = NULL;
    int rc;
    sqlite3_stmt *stmt = NULL;
    RinksGame *game;

    int col_count, col;

    rc = sqlite3_prepare_v2(db_handle, "select * from games order by id desc", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        goto out;

    col_count = sqlite3_column_count(stmt);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        game = g_malloc0(sizeof(RinksGame));
        for (col = 0; col < col_count; ++col) {
#define DBGETVAL(name, type, val) if (db_sql_extract_value(stmt, col, (name), (type), &(val))) continue
            DBGETVAL("id", G_TYPE_INT64, game->id);
            DBGETVAL("description", G_TYPE_STRING, game->description);
            DBGETVAL("sequence", G_TYPE_INT, game->sequence);
            DBGETVAL("closed", G_TYPE_INT, game->closed);
#undef DBGETVAL
        }

        result = g_list_prepend(result, game);
    }

out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return result;
}

RinksGame *db_get_game(gpointer db_handle, gint64 game_id)
{
    g_return_val_if_fail(db_handle != NULL, NULL);

    g_printf("db get game %" G_GINT64_FORMAT, game_id);

    if (game_id <= 0)
        return NULL;

    int rc;
    sqlite3_stmt *stmt = NULL;
    RinksGame *game = NULL;

    int col_count, col;
#ifdef WIN32
    gchar *sql = sqlite3_mprintf("select * from games where id=%lld", game_id);
#else
    gchar *sql = sqlite3_mprintf("select * from games where id=%" G_GINT64_FORMAT, game_id);
#endif

    rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        goto out;

    col_count = sqlite3_column_count(stmt);

    if ((rc = sqlite3_step(stmt)) != SQLITE_ROW)
        goto out;

    game = g_malloc0(sizeof(RinksGame));
    for (col = 0; col < col_count; ++col) {
#define DBGETVAL(name, type, val) if (db_sql_extract_value(stmt, col, (name), (type), &(val))) continue
        DBGETVAL("id", G_TYPE_INT64, game->id);
        DBGETVAL("description", G_TYPE_STRING, game->description);
        DBGETVAL("sequence", G_TYPE_INT, game->sequence);
        DBGETVAL("closed", G_TYPE_INT, game->closed);
#undef DBGETVAL
    }

out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return game;
}

void db_update_game(gpointer db_handle, RinksGame *game)
{
    g_return_if_fail(db_handle != NULL);
    g_return_if_fail(game != NULL);

    g_printf("db: update game %" G_GINT64_FORMAT, game->id);
    if (game->id <= 0)
        return;

    int rc;
#ifdef WIN32
    gchar *sql = sqlite3_mprintf("update games set description=%Q, closed=%d, sequence=%d where id=%lld",
            game->description, game->closed, game->sequence, game->id);
#else
    gchar *sql = sqlite3_mprintf("update games set description=%Q, closed=%d, sequence=%d where id=%" G_GINT64_FORMAT,
            game->description, game->closed, game->sequence, game->id);
#endif

    rc = sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        return;
}

void db_set_result(gpointer db_handle, RinksResult *result)
{
    g_return_if_fail(db_handle != NULL);
    g_return_if_fail(result != NULL);
    
    int rc;
    gchar *sql;
#ifdef WIN32
    sql = sqlite3_mprintf("update results set points=%d, ends=%d, stones=%d where team=%lld and encounter=%lld",
                result->points, result->ends, result->stones, result->team, result->encounter);
#else
    sql = sqlite3_mprintf("update results set points=%d, ends=%d, stones=%d where team=%" G_GINT64_FORMAT " and encounter=%" G_GINT64_FORMAT,
                result->points, result->ends, result->stones, result->team, result->encounter);
#endif

    rc = sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        return;

    if (sqlite3_changes(db_handle) == 0) {
#ifdef WIN32
        sql = sqlite3_mprintf("insert into results (team,encounter,points,ends,stones) values (\
%lld, %lld, %d, %d, %d)",
                result->team, result->encounter, result->points, result->ends, result->stones);
#else
        sql = sqlite3_mprintf("insert into results (team,encounter,points,ends,stones) values (\
%" G_GINT64_FORMAT ", %" G_GINT64_FORMAT ", %d, %d, %d)",
                result->team, result->encounter, result->points, result->ends, result->stones);
#endif
        sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
        sqlite3_free(sql);
    }
}

RinksResult *db_get_result(gpointer db_handle, gint64 encounter, gint64 team)
{
    g_return_val_if_fail(db_handle != NULL, NULL);

    int rc;
    sqlite3_stmt *stmt = NULL;
    RinksResult *result = NULL;

    int col_count, col;

#ifdef WIN32
    gchar *sql = sqlite3_mprintf("select results.*, encounters.round, encounters.game from results left outer join encounters on results.encounter=encounters.id where encounter=%lld and team=%lld",
            encounter, team);
#else
    gchar *sql = sqlite3_mprintf("select results.*, encounters.round, encounters.game from results left outer join encounters on results.encounter=encounters.id where encounter=%" G_GINT64_FORMAT " and team=%" G_GINT64_FORMAT ,
            encounter, team);
#endif
    rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        goto out;

    col_count = sqlite3_column_count(stmt);

    if ((rc = sqlite3_step(stmt)) != SQLITE_ROW)
        goto out;

    result = g_malloc0(sizeof(RinksResult));
    for (col = 0; col < col_count; ++col) {
#define DBGETVAL(name, type, val) if (db_sql_extract_value(stmt, col, (name), (type), &(val))) continue
        DBGETVAL("id", G_TYPE_INT64, result->id);
        DBGETVAL("encounter", G_TYPE_INT64, result->encounter);
        DBGETVAL("team", G_TYPE_INT64, result->team);
        DBGETVAL("points", G_TYPE_INT, result->points);
        DBGETVAL("ends", G_TYPE_INT, result->ends);
        DBGETVAL("stones", G_TYPE_INT, result->stones);
        DBGETVAL("round", G_TYPE_INT64, result->round);
        DBGETVAL("game", G_TYPE_INT64, result->game);
#undef DBGETVAL
    }

out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return result;
}

GList *db_get_team_results(gpointer db_handle, gint64 team)
{
    g_return_val_if_fail(db_handle != NULL, NULL);

    int rc;
    GList *result = NULL;
    sqlite3_stmt *stmt = NULL;
    RinksResult *res;

    int col_count, col;

    gchar *sql;
   
    if (team > 0) {
#ifdef WIN32
        sql = sqlite3_mprintf("select results.*, encounters.round, encounters.game from results \
left outer join encounters on results.encounter=encounters.id where team=%lld order by encounter desc", team);
#else
        sql = sqlite3_mprintf("select results.*, encounters.round, encounters.game from results \
left outer join encounters on results.encounter=encounters.id where team=%" G_GINT64_FORMAT " order by encounter desc", team);
#endif
    }
    else
        sql = sqlite3_mprintf("select results.*, encounters.round, encounters.game from results left outer join encounters on \
results.encounter=encounters.id order by encounter desc");

    rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        goto out;

    col_count = sqlite3_column_count(stmt);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        res = g_malloc0(sizeof(RinksResult));
        for (col = 0; col < col_count; ++col) {
#define DBGETVAL(name, type, val) if (db_sql_extract_value(stmt, col, (name), (type), &(val))) continue
        DBGETVAL("id", G_TYPE_INT64, res->id);
        DBGETVAL("encounter", G_TYPE_INT64, res->encounter);
        DBGETVAL("team", G_TYPE_INT64, res->team);
        DBGETVAL("points", G_TYPE_INT, res->points);
        DBGETVAL("ends", G_TYPE_INT, res->ends);
        DBGETVAL("stones", G_TYPE_INT, res->stones);
        DBGETVAL("round", G_TYPE_INT64, res->round);
        DBGETVAL("game", G_TYPE_INT64, res->game);
#undef DBGETVAL
        }

        result = g_list_prepend(result, res);
    }

out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return result;
}

gint64 db_add_override(gpointer db_handle, RinksOverride *override)
{
    g_return_val_if_fail(db_handle != NULL, -1);
    g_return_val_if_fail(override != NULL, -1);

    int rc;

#ifdef WIN32
    gchar *sql = sqlite3_mprintf("insert into overrides (encounter,team1,team2) values (%lld, %lld, %lld)",
            override->encounter, override->team1, override->team2);
#else
    gchar *sql = sqlite3_mprintf("insert into overrides (encounter,team1,team2) values (%"\
G_GINT64_FORMAT ", %" G_GINT64_FORMAT ", %" G_GINT64_FORMAT ")",
            override->encounter, override->team1, override->team2);
#endif

    rc = sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        return -1;
    }

    return sqlite3_last_insert_rowid(db_handle);
}

void db_update_override(gpointer db_handle, RinksOverride *override)
{
    g_return_if_fail(db_handle != NULL);
    g_return_if_fail(override != NULL);

    int rc;
#ifdef WIN32
    gchar *sql = sqlite3_mprintf("update overrides set encounter=%lld, team1=%lld, team2=%lld where id=%lld",
            override->encounter, override->team1, override->team2, override->id);
#else
    gchar *sql = sqlite3_mprintf("update overrides set encounter=%" G_GINT64_FORMAT ", team1=%" G_GINT64_FORMAT\
", team2=%" G_GINT64_FORMAT " where id=%" G_GINT64_FORMAT,
            override->encounter, override->team1, override->team2, override->id);
#endif

    rc = sqlite3_exec(db_handle, sql, NULL, NULL, NULL);
    sqlite3_free(sql);

    if (rc != SQLITE_OK)
        return;
}

GList *db_get_overrides(gpointer db_handle)
{
    g_return_val_if_fail(db_handle != NULL, NULL);

    GList *result = NULL;
    int rc;
    sqlite3_stmt *stmt = NULL;
    RinksOverride *override;

    int col_count, col;

    char *sql = "select overrides.*, encounters.round from overrides left outer join encounters on overrides.encounter=encounters.id order by id desc";

    rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK)
        goto out;

    col_count = sqlite3_column_count(stmt);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        override = g_malloc0(sizeof(RinksOverride));
        for (col = 0; col < col_count; ++col) {
#define DBGETVAL(name, type, val) if (db_sql_extract_value(stmt, col, (name), (type), &(val))) continue
            DBGETVAL("id", G_TYPE_INT64, override->id);
            DBGETVAL("encounter", G_TYPE_INT64, override->encounter);
            DBGETVAL("team1", G_TYPE_INT64, override->team1);
            DBGETVAL("team2", G_TYPE_INT64, override->team2);
            DBGETVAL("round", G_TYPE_INT64, override->round);
#undef DBGETVAL
        }

        result = g_list_prepend(result, override);
    }
out:
    if (stmt != NULL)
        sqlite3_finalize(stmt);
    return result;
}
