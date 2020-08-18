#include <stdio.h>
#include <errno.h>
#include <string.h>

#define MAX_LINE_LENGTH     512
#define MAX_KEY_LENGTH      128
#define MAX_VALUE_LENGTH    128

struct param_value {
    char param[128];
    char value[128];
};

int init_config(const char* config_filename, char *mode);
void stop_config();

void config_set_handler(void (*f)(const char* key, const char* value));
void config_parse();
