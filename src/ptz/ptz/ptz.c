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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include "ptz.h"
#include "config.h"
#include "libptz.h"

struct preset presets[PRESET_NUM];
int debug;

void print_usage(char *progname)
{
    fprintf(stderr, "\nUsage: %s OPTIONS\n\n", progname);
    fprintf(stderr, "\t-a ACTION, --action ACTION\n");
    fprintf(stderr, "\t\tset PTZ action: stop, right, left, down, up, go, set_preset, go_preset\n");
    fprintf(stderr, "\t-t TIME, --time TIME\n");
    fprintf(stderr, "\t\tset action duration in milliseconds (default 500)\n");
    fprintf(stderr, "\t-x X, --x X\n");
    fprintf(stderr, "\t\tset X coordinate when using GO action\n");
    fprintf(stderr, "\t-y Y, --y Y\n");
    fprintf(stderr, "\t\tset Y coordinate when using GO action\n");
    fprintf(stderr, "\t-n NUM, --preset_num NUM\n");
    fprintf(stderr, "\t\tset preset NUM (0-14) when using set_preset or go_preset actions\n");
    fprintf(stderr, "\t\tspecial presets: 10 center\n");
    fprintf(stderr, "\t\t                 11 upper left corner\n");
    fprintf(stderr, "\t\t                 12 upper right corner\n");
    fprintf(stderr, "\t\t                 13 lower right corner\n");
    fprintf(stderr, "\t\t                 14 lower left corner\n");
    fprintf(stderr, "\t-c, --clear\n");
    fprintf(stderr, "\t\tclear preset (used with set_preset action)\n");
    fprintf(stderr, "\t-e DESCRIPTION, --desc DESCRIPTION\n");
    fprintf(stderr, "\t\tset description (used with set_preset action)\n");
    fprintf(stderr, "\t-d, --debug\n");
    fprintf(stderr, "\t\tenable debug\n");
    fprintf(stderr, "\t-h, --help\n");
    fprintf(stderr, "\t\tprint this help\n");
}

void handle_config(char *key, char *value)
{
    char *p;
    int nvalue;
    int ikey;

    if (debug) fprintf(stderr, "key=%s, value=%s\n", key, value);

    errno = 0;
    nvalue = strtol(key, NULL, 10);
    if (errno == 0) {
        ikey = nvalue;
    } else {
        printf("Error reading configuration file\n");
        exit(EXIT_FAILURE);
    }

    p = strtok(value, "|");
    if (p != NULL) {
        strcpy(presets[ikey].desc, p);
        p = strtok(NULL, "|");
        if (p != NULL) {
            errno = 0;
            nvalue = strtol(p, NULL, 10);
            if (errno == 0) {
                presets[ikey].x = nvalue;
            } else {
                printf("Error reading configuration file\n");
                exit(EXIT_FAILURE);
            }
            p = strtok(NULL, "|");
            if (p != NULL) {
                errno = 0;
                nvalue = strtol(p, NULL, 10);
                if (errno == 0) {
                    presets[ikey].y = nvalue;
                } else {
                    printf("Error reading configuration file\n");
                    exit(EXIT_FAILURE);
                }
                p = strtok(NULL, "|");
                if (p != NULL) {
                    printf("Error reading configuration file\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                printf("Error reading configuration file\n");
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Error reading configuration file\n");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Error reading configuration file\n");
        exit(EXIT_FAILURE);
    }
}

int ptz_init_config(char *filename)
{
    if (init_config(filename, "r") != 0)
    {
        fprintf(stderr, "Cannot open config file.\n");
        return -1;
    }

    config_set_handler(&handle_config);
    config_parse();
    stop_config();

    return 0;
}

int ptz_save_config(char *filename)
{
    int i;

    if (init_config(filename, "w") != 0)
    {
        fprintf(stderr, "Cannot open config file.\n");
        return -1;
    }

    config_save(presets, PRESET_NUM);
    stop_config();

    return 0;
}

int main(int argc, char **argv)
{
    int action;
    int time;
    int x, y;
    char preset_file[1024];
    int preset_num;
    int clear;
    int c;
    char desc[256];
    int errno;
    char *endptr;
    int ptz_arg[8];
    int i;
    int preset_buffer[10];

    // Setting default
    action = ACTION_NONE;
    time = DEFAULT_ACTION_TIME;
    x = 0;
    y = 0;
    preset_file[0] = '\0';
    preset_num = -1;
    clear = 0;
    debug = 0;

    while (1) {
        static struct option long_options[] =
        {
            {"action",  required_argument, 0, 'a'},
            {"time",  required_argument, 0, 't'},
            {"x",  required_argument, 0, 'x'},
            {"y",  required_argument, 0, 'y'},
            {"preset_file",  required_argument, 0, 'f'},
            {"preset_num",  required_argument, 0, 'n'},
            {"desc",  required_argument, 0, 'e'},
            {"clear",  no_argument, 0, 'c'},
            {"debug",  no_argument, 0, 'd'},
            {"help",  no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "a:t:x:y:f:n:e:cdh",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {

        case 'a':
            if (strcasecmp("stop", optarg) == 0) {
                action = ACTION_STOP;
            } else if (strcasecmp("right", optarg) == 0) {
                action = ACTION_RIGHT;
            } else if (strcasecmp("left", optarg) == 0) {
                action = ACTION_LEFT;
            } else if (strcasecmp("down", optarg) == 0) {
                action = ACTION_DOWN;
            } else if (strcasecmp("up", optarg) == 0) {
                action = ACTION_UP;
            } else if (strcasecmp("go", optarg) == 0) {
                action = ACTION_GO;
            } else if (strcasecmp("set_preset", optarg) == 0) {
                action = ACTION_SET_PRESET;
            } else if (strcasecmp("go_preset", optarg) == 0) {
                action = ACTION_GO_PRESET;
            }
            break;

        case 't':
            errno = 0;    /* To distinguish success/failure after call */
            time = strtol(optarg, &endptr, 10);

            /* Check for various possible errors */
            if ((errno == ERANGE) && (time == LONG_MAX || time == LONG_MIN)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if (endptr == optarg) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if ((time < 0) || (time > 5000)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'x':
            errno = 0;    /* To distinguish success/failure after call */
            x = strtol(optarg, &endptr, 10);

            /* Check for various possible errors */
            if ((errno == ERANGE) && (x == LONG_MAX || x == LONG_MIN)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if (endptr == optarg) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if ((x < MIN_X) || (x > MAX_X)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'y':
            errno = 0;    /* To distinguish success/failure after call */
            y = strtol(optarg, &endptr, 10);

            /* Check for various possible errors */
            if ((errno == ERANGE) && (y == LONG_MAX || y == LONG_MIN)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if (endptr == optarg) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if ((y < MIN_Y) || (y > MAX_Y)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'f':
            strcpy(preset_file, optarg);
            break;

        case 'n':
            errno = 0;    /* To distinguish success/failure after call */
            preset_num = strtol(optarg, &endptr, 10);

            /* Check for various possible errors */
            if ((errno == ERANGE) && (preset_num == LONG_MAX || preset_num == LONG_MIN)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if (endptr == optarg) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if ((preset_num < 0) || (preset_num > 14)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'e':
            if (strlen(optarg) > sizeof(desc) - 1) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            strcpy(desc, optarg);
            break;

        case 'c':
            clear = 1;
            break;

        case 'd':
            fprintf (stderr, "debug on\n");
            debug = 1;
            break;

        case 'h':
            print_usage(argv[0]);
            return -1;
            break;

        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            print_usage(argv[0]);
            return -1;
        }
    }

    if (action == ACTION_NONE) {
        print_usage(argv[0]);
        return -1;
    }
    if (((action == ACTION_SET_PRESET) || (action == ACTION_GO_PRESET)) &&
            (preset_num == -1)) {
        printf("preset_num cannot be empty.\n");
        print_usage(argv[0]);
        return -1;
    }
    if ((action == ACTION_SET_PRESET) && (preset_num > 9)) {
        printf("preset_num must be between 0 and 9.\n");
        print_usage(argv[0]);
        return -1;
    }
    if (((action == ACTION_SET_PRESET) || (action == ACTION_GO_PRESET)) &&
            (preset_file[0] == '\0')) {
        printf("preset_file cannot be empty.\n");
        print_usage(argv[0]);
        return -1;
    }
    if ((clear == 1) && (action != ACTION_SET_PRESET)) {
        printf("clear flag must be used with set_preset action.\n");
        print_usage(argv[0]);
        return -1;
    }

    if (debug) fprintf(stderr, "Running action %d\n", action);

    if (action == ACTION_SET_PRESET) {
        ptz_init_config(preset_file);
        memset(preset_buffer, '\0', sizeof(preset_buffer));
        if (clear == 1) {
            x = -1;
            y = -1;
            strcpy(desc, "none");
        } else {
            if (hw_ptz_pos_read(0, preset_buffer, 0, 0) != 0) {
                printf("Error reading position\n");
                return -2;
            }
            x = preset_buffer[3];
            y = preset_buffer[4];
            if (debug) fprintf(stderr, "Current position: (%d, %d)\n", x, y);
        }

        presets[preset_num].x = x;
        presets[preset_num].y = y;
        strcpy(presets[preset_num].desc, desc);
        ptz_save_config(preset_file);

        return 0;
    }

    if (action == ACTION_GO_PRESET) {
        if (preset_num <= 9) {
            ptz_init_config(preset_file);
            if (debug) {
                for (i=0; i<PRESET_NUM; i++) {
                    fprintf(stderr, "CONF: %d - %s, %d, %d\n", i, presets[i].desc, presets[i].x, presets[i].y);
                }
            }
            x = presets[preset_num].x;
            y = presets[preset_num].y;
        } else {
            switch (preset_num) {
                case 10:
                    x = MAX_X / 2;
                    y = MAX_Y / 2;
                    break;
                case 11:
                    x = MAX_X;
                    y = MAX_Y;
                    break;
                case 12:
                    x = MIN_X;
                    y = MAX_Y;
                    break;
                case 13:
                    x = MIN_X;
                    y = MIN_Y;
                    break;
                case 14:
                    x = MAX_X;
                    y = MIN_Y;
                    break;
            }
        }
        if ((x == -1) || (y == -1)) {
            if (debug) fprintf(stderr, "No valid preset %d\n", preset_num);
            return 0;
        } else {
            if (debug) fprintf(stderr, "Go to preset %d: (%d, %d)\n", preset_num, presets[0].x, presets[0].y);
        }
    }

    ptz_arg[0] = action % 100;
    ptz_arg[1] = 0;
    ptz_arg[2] = 1; // 1, 2 or 3
    ptz_arg[3] = x; // absolute position x
    ptz_arg[4] = y; // absolute position y
    ptz_arg[5] = 0; // ptz crz
    ptz_arg[6] = 0;
    ptz_arg[7] = 0;
    hw_ptz_sendptz(ptz_arg);

    // Send stop command
    if ((action == ACTION_RIGHT) || (action == ACTION_LEFT) ||
             (action == ACTION_DOWN) || (action == ACTION_UP)) {

        usleep(time * 1000);

        if (debug) fprintf(stderr, "running stop\n");
        ptz_arg[0] = ACTION_STOP;
        hw_ptz_sendptz(ptz_arg);
    }

    return 0;
}
