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

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH     512
#define MAX_KEY_LENGTH      128
#define MAX_VALUE_LENGTH    128

int init_config(const char* config_filename);
void stop_config();

void config_set_handler(void (*f)(const char* key, const char* value));
void config_parse();

void conf_set_double(const char* value, double* conf);
void conf_set_int(const char* value, int* conf);
char *conf_set_string(const char* value);
char *conf_set_strings(const char* value1, const char* value2);
char *get_conf_file_single(const char* filename);

#endif // CONFIG_H
