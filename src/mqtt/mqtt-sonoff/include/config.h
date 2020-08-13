#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define MAX_LINE_LENGTH     512
#define MAX_KEY_LENGTH      128
#define MAX_VALUE_LENGTH    128

int init_config(const char* config_filename);
void stop_config();

void config_set_handler(void (*f)(const char* key, const char* value));
void config_parse();

#endif // CONFIG_H
