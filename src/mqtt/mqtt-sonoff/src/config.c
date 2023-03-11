/*
 * Copyright (c) 2023 roleo.
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

#include "config.h"

static int open_conf_file(const char* filename);

static void (*fconf_handler)(const char* key, const char* value);

static FILE *fp;

int init_config(const char* config_filename)
{
    if(open_conf_file(config_filename)!=0)
        return -1;

    return 0;
}

void stop_config()
{
    if(fp!=NULL)
        fclose(fp);
}

void config_set_handler(void (*f)(const char* key, const char* value))
{
    if(f!=NULL)
        fconf_handler=f;
}

void config_parse()
{
    char buf[MAX_LINE_LENGTH];
    char key[128];
    char value[128];

    int parsed;

    if(fp==NULL)
        return;

    while(fgets(buf, MAX_LINE_LENGTH, fp))
    {
        if(buf[0]!='#') // ignore the comments
        {
            parsed=sscanf(buf, "%[^=] = %s", key, value);
            if(parsed==2 && fconf_handler!=NULL)
                (*fconf_handler)(key, value);
        }
    }
}

static int open_conf_file(const char* filename)
{
    fp=fopen(filename, "r");
    if(fp==NULL)
    {
        fprintf(stderr, "Can't open file \"%s\": %s\n", filename, strerror(errno));
        return -1;
    }

    return 0;
}

void conf_set_double(const char* value, double* conf){
    double dvalue;

    errno=0;
    dvalue=strtod(value, NULL);
    if(errno==0)
        *conf=dvalue;
}

void conf_set_int(const char* value, int* conf){
    int nvalue;

    errno=0;
    nvalue=strtol(value, NULL, 10);
    if(errno==0)
        *conf=nvalue;
}

char *conf_set_string(const char* value){
    char *conf;
    conf = malloc(strlen(value)+1);
    //strcpy will probably segfault if conf is null
    if (conf == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }

    strcpy(conf, value);

    return conf;
}

char *conf_set_strings(const char* value1, const char* value2){
    char *conf;
    conf = malloc(strlen(value1)+strlen(value2)+1);
    //strcpy will probably segfault if conf is null
    if (conf == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }

    sprintf(conf, "%s%s", value1, value2);

    return conf;
}

char *get_conf_file_single(const char* filename)
{
    char buf[MAX_LINE_LENGTH];
    FILE *fps;

    fps=fopen(filename, "r");
    if(fps==NULL)
    {
        fprintf(stderr, "Can't open file \"%s\": %s\n", filename, strerror(errno));
        return NULL;
    }

    while(fgets(buf, MAX_LINE_LENGTH, fps))
    {
        if(buf[0]!='#') // ignore comments
        {
            buf[strcspn(buf, "\n")] = 0;
            fclose(fps);
            return conf_set_string(buf);
        }
    }
    return NULL;
}