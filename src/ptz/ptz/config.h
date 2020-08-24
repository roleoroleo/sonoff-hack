#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "ptz.h"

#define MAX_LINE_LENGTH     512
#define MAX_KEY_LENGTH      128
#define MAX_VALUE_LENGTH    128

int init_config(const char* config_filename, char *mode);
void stop_config();

void config_save(struct preset pr[], int size);

void config_set_handler(void (*f)(char* key, char* value));
void config_parse();

#endif //CONFIG_H