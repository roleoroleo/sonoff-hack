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

#endif // CONFIG_H
