/*
 * This file is part of libipc (https://github.com/TheCrypt0/libipc).
 * Copyright (c) 2019 Davide Maggioni.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "sql.h"

//-----------------------------------------------------------------------------
// GENERAL STATIC VARS AND FUNCTIONS
//-----------------------------------------------------------------------------

static pthread_t *tr_sql;
int tr_sql_routine;
int ipcsys_db = 1;
sqlite3 *dbc = NULL;
char last_msg[32];

static int start_sql_thread();
static void *sql_thread(void *args);
static int parse_message(char *msg);

static void call_callback(SQL_MESSAGE_TYPE type);
static void sql_debug(const char* fmt, ...);

//-----------------------------------------------------------------------------
// MESSAGES HANDLERS
//-----------------------------------------------------------------------------

static void handle_sql_motion_start();

//-----------------------------------------------------------------------------
// FUNCTION POINTERS TO CALLBACKS
//-----------------------------------------------------------------------------

typedef void(*func_ptr_t)(void);

static func_ptr_t *sql_callbacks;

//=============================================================================

//-----------------------------------------------------------------------------
// INIT
//-----------------------------------------------------------------------------

int sql_init(int sysdb)
{
    int ret = 0;
    ipcsys_db = sysdb;

    memset(last_msg, '\0', sizeof(last_msg));
    if (ipcsys_db) {
        ret = sqlite3_open_v2(IPCSYS_DB, &dbc, SQLITE_OPEN_READONLY, NULL);
    } else {
        ret = sqlite3_open_v2(IPCMMC_DB, &dbc, SQLITE_OPEN_READONLY, NULL);
    }

    if (ret != SQLITE_OK) {
        fprintf(stderr, "Error opening db\n");
        return -1;
    }

    ret = start_sql_thread();
    if(ret != 0)
        return -2;

    sql_callbacks = malloc((sizeof(func_ptr_t)) * SQL_MSG_LAST);

    return 0;
}

void sql_stop()
{
    if(tr_sql != NULL)
    {
        tr_sql_routine = 0;
        pthread_join(*tr_sql, NULL);
        free(tr_sql);
    }

    if(sql_callbacks != NULL)
        free(sql_callbacks);

    if (dbc) {
        sqlite3_close_v2(dbc);
    }

}

static int start_sql_thread()
{
    int ret;

    tr_sql = malloc(sizeof(pthread_t));
    tr_sql_routine = 1;
    ret = pthread_create(tr_sql, NULL, &sql_thread, NULL);
    if(ret != 0)
    {
        fprintf(stderr, "Can't create sql thread. Error: %d\n", ret);
        return -1;
    }

    return 0;
}

static void *sql_thread(void *args)
{
    sqlite3_stmt *stmt = NULL;
    int ret = 0;
    char buffer[1024];
    if (ipcsys_db) {
        sprintf (buffer, "select max(c_alarm_time), c_alarm_context from t_alarm_log;");
    } else {
        sprintf (buffer, "select max(c_alarm_time), c_alarm_code from T_RecordFile;");
    }
    ret = sqlite3_prepare_v2(dbc, buffer, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(dbc));
    }

    while(tr_sql_routine)
    {
        ret = sqlite3_step(stmt);
        if (ret == SQLITE_ROW) {
            if (sqlite3_column_text(stmt, 0) != NULL) {
                parse_message((char *) sqlite3_column_text(stmt, 0));
            }
        }
        ret = sqlite3_reset(stmt);
        usleep(1000*1000);
    }

    sqlite3_finalize(stmt);

    return 0;
}

//-----------------------------------------------------------------------------
// MSG PARSER
//-----------------------------------------------------------------------------

static int parse_message(char *msg)
{
    sql_debug("Parsing message.\n");

    sql_debug(msg);
    sql_debug("\n");

    if (strcmp(last_msg, msg) != 0) {
        if (last_msg[0] != '\0') {
            handle_sql_motion_start();
        }
        strcpy(last_msg, msg);
        return 0;
    }

    return -1;
}

//-----------------------------------------------------------------------------
// HANDLERS
//-----------------------------------------------------------------------------

static void handle_sql_motion_start()
{
    sql_debug("GOT MOTION START\n");
    call_callback(SQL_MSG_MOTION_START);
}

//-----------------------------------------------------------------------------
// GETTERS AND SETTERS
//-----------------------------------------------------------------------------

int sql_set_callback(SQL_MESSAGE_TYPE type, void (*f)())
{
    if(type >= SQL_MSG_LAST)
        return -1;

    sql_callbacks[(int)type]=f;

    return 0;
}

//-----------------------------------------------------------------------------
// UTILS
//-----------------------------------------------------------------------------

static void call_callback(SQL_MESSAGE_TYPE type)
{
    func_ptr_t f;
    // Not handling callbacks with parameters (yet)
    f = sql_callbacks[(int)type];
    if(f != NULL)
        (*f)();
}

static void sql_debug(const char* fmt, ...)
{
#if SQL_DEBUG
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
}
