#include "config.h"

static int open_conf_file(const char* filename, char *mode);

static void (*fconf_handler)(const char* key, const char* value);

static FILE *fp;

int init_config(const char* config_filename, char *mode)
{
    if (open_conf_file(config_filename, mode) != 0)
        return -1;

    return 0;
}

void stop_config()
{
    if (fp != NULL)
        fclose(fp);
}

void config_set_handler(void (*f)(const char* key, const char* value))
{
    if (f != NULL)
        fconf_handler = f;
}

void config_parse()
{
    char buf[MAX_LINE_LENGTH];
    char key[128];
    char value[128];

    int parsed;

    if(fp == NULL)
        return;

    while (!feof(fp)) {
        fgets(buf, MAX_LINE_LENGTH, fp);
        if (buf[0] != '#') // ignore the comments
        {
            parsed = sscanf(buf, "%[^=] = %s", key, value);
            if(parsed == 2 && fconf_handler != NULL)
                (*fconf_handler)(key, value);
        }
    }
}

void config_save(struct param_value config[], int size)
{
    int i;

    for (i=0; i<size; i++) {
        fprintf(fp, "%s=%s\n", config[i].param, config[i].value);
    }
}

static int open_conf_file(const char* filename, char *mode)
{
    fp = fopen(filename, mode);
    if(fp == NULL) {
        fprintf(stderr, "Can't open file \"%s\": %s\n", filename, strerror(errno));
        return -1;
    }

    return 0;
}
