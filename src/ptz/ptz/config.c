#include "config.h"

static int open_conf_file(const char* filename, char *mode);

static void (*fconf_handler)(char* key, char* value);

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

void config_set_handler(void (*f)(char* key, char* value))
{
    if (f != NULL)
        fconf_handler = f;
}

void config_parse()
{
    char buf[MAX_LINE_LENGTH];
    char key[128];
    char value[128];
    char *p;

    int parsed;

    if(fp == NULL)
        return;

    while (!feof(fp)) {
        fgets(buf, MAX_LINE_LENGTH, fp);
        if (buf[0] != '#') // ignore the comments
        {
            p = strtok(buf, " =\n");
            if (p != NULL) {
                strcpy(key, p);
                p = strtok(NULL, "=\n");
                if (p != NULL) {
                    strcpy(value, p);
                    (*fconf_handler)(key, value);
                }
            }
        }
    }
}

void config_save(struct preset pr[], int size)
{
    int i;

    for (i=0; i<size; i++) {
        fprintf(fp, "%d=%s|%d|%d\n", i, pr[i].desc, pr[i].x, pr[i].y);
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
