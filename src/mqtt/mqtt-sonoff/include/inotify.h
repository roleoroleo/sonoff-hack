/*
 * Copyright (c) 2023 roleo.
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

#ifndef INOTIFY_H
#define INOTIFY_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <poll.h>

#define TMP_DIR   "/tmp"
#define NOTI_FILE "colinkPushNotice"

typedef enum
{
    INOTIFY_MSG_MOTION_START,
    INOTIFY_MSG_LAST
} INOTIFY_MESSAGE_TYPE;

//-----------------------------------------------------------------------------
// INIT
//-----------------------------------------------------------------------------

int inotify_start();
void inotify_stop();

//-----------------------------------------------------------------------------
// GETTERS AND SETTERS
//-----------------------------------------------------------------------------

int inotify_set_callback(INOTIFY_MESSAGE_TYPE type, void (*f)());

#endif // INOTIFY_H
