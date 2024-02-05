/*
 * Copyright (c) 2020 roleo.
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

/*
 * Send PTZ commands to /dev/ptz using libptz.so
 */

#ifndef PTZ_H
#define PTZ_H

#define ACTION_NONE            -1
#define ACTION_STOP            0
#define ACTION_RIGHT           1
#define ACTION_LEFT            2
#define ACTION_DOWN            3
#define ACTION_UP              4
#define ACTION_GO              14
#define ACTION_GO_REL          114
#define ACTION_GO_PRESET       214

#define ACTION_GET_PRESETS     300
#define ACTION_SET_PRESET      301
#define ACTION_DEL_PRESET      302
#define ACTION_SET_HOME        303

#define ACTION_GET_COORD       400

#define DEFAULT_ACTION_TIME    500

#define MAXLINE                100

#define PRESET_NUM             10

#define MIN_X                  0
#define MAX_X                  3500
#define MIN_Y                  0
#define MAX_Y                  1350

#define MIN_X_DEG              0.0
#define MAX_X_DEG              360.0
#define MIN_Y_DEG              0.0
#define MAX_Y_DEG              180.0

struct preset {
    int x;
    int y;
    char desc[256];
};

#endif //PTZ_H
