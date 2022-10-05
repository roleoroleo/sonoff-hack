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

#define SQL_DEBUG             0

#define IPCSYS_DB             "/mnt/mtd/db/ipcsys.db"
#define IPCMMC_DB             "/mnt/mmc/AVRecordFile.db"

typedef enum
{
    SQL_MSG_UNRECOGNIZED,
    SQL_MSG_MOTION_START,
    SQL_MSG_LAST
} SQL_MESSAGE_TYPE;

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
