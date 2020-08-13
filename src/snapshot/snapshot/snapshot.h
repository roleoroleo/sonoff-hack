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
 * Create a jpg snapshot.
 */

#define MSG_SIZE 288

#define MSG_PART_1 "\x44\x59\x30\x31\x18\x01\x00\x00\xF1\x03\x00\x00\x88\x77\xB0\x7E\x88\x12\xF6\x76\xB8\x7D\xB0\x7E"
#define MSG_PART_3 "\x01\x01\xB0\x7E"

#define LOCAL_IP "127.0.0.1"
#define AVENCODE_IP "127.0.0.1"
#define AVENCODE_PORT 11000
