/*
 * Copyright (c) 2025 roleo.
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

#ifndef SQL_H
#define SQL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <mqueue.h>
#include <sqlite3.h>

#define IPCSYS_DB             "/mnt/mtd/db/ipcsys.db"
#define IPCMMC_DB             "/mnt/mmc/AVRecordFile.db"

typedef enum
{
    SQL_MSG_UNRECOGNIZED,
    SQL_MSG_MOTION_START,
    SQL_MSG_COMMAND,
    SQL_MSG_LAST
} SQL_MESSAGE_TYPE;

typedef enum
{
    SQL_CMD_SENSITIVITY_0,
    SQL_CMD_SENSITIVITY_25,
    SQL_CMD_SENSITIVITY_50,
    SQL_CMD_SENSITIVITY_75,
    SQL_CMD_MOTION_DETECTION_ENABLED,
    SQL_CMD_MOTION_DETECTION_DISABLED,
    SQL_CMD_LOCAL_RECORD_ENABLED,
    SQL_CMD_LOCAL_RECORD_DISABLED,
    SQL_CMD_IR_AUTO,
    SQL_CMD_IR_ON,
    SQL_CMD_IR_OFF,
    SQL_CMD_ROTATE_ENABLED,
    SQL_CMD_ROTATE_DISABLED,
    PRIVACY_OFF,
    PRIVACY_ON,
    SQL_CMD_LAST
} SQL_COMMAND_TYPE;

//-----------------------------------------------------------------------------
// INIT
//-----------------------------------------------------------------------------

int sql_init(int);
void sql_stop();

//-----------------------------------------------------------------------------
// GETTERS AND SETTERS
//-----------------------------------------------------------------------------

int sql_set_callback(SQL_MESSAGE_TYPE type, void (*f)());

#endif // SQL_H
