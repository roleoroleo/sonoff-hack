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

#define ACTION_STOP    0
#define ACTION_RIGHT   1
#define ACTION_LEFT    2
#define ACTION_DOWN    3
#define ACTION_UP      4

#define ACTION_TIME    500

int hw_ptz_sendptz(int *ptz_arg);
