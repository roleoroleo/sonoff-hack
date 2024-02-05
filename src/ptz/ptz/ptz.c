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
#include <math.h>
#include "ptz.h"
#include "config.h"
#include "libptz.h"

struct preset presets[PRESET_NUM];
int debug;

void print_usage(char *progname)
{
    fprintf(stderr, "\nUsage: %s OPTIONS\n\n", progname);
    fprintf(stderr, "\t-a ACTION, --action ACTION\n");
    fprintf(stderr, "\t\tset PTZ action: stop, right, left, down, up, go, go_rel, get_presets, go_preset, set_preset, del_preset, set_home, get_coord\n");
    fprintf(stderr, "\t-t TIME, --time TIME\n");
    fprintf(stderr, "\t\tset action duration in milliseconds (right, left, down and up)\n");
    fprintf(stderr, "\t\tdefault 500\n");
    fprintf(stderr, "\t-x X, --x X\n");
    fprintf(stderr, "\t\tset X coordinate when using GO or GO_REL action\n");
    fprintf(stderr, "\t-y Y, --y Y\n");
    fprintf(stderr, "\t\tset Y coordinate when using GO or GO_REL action\n");
    fprintf(stderr, "\t-X X, --X X\n");
    fprintf(stderr, "\t\tset X coordinate (degrees) when using GO or GO_REL action\n");
    fprintf(stderr, "\t-Y Y, --Y Y\n");
    fprintf(stderr, "\t\tset Y coordinate (degrees) when using GO or GO_REL action\n");
    fprintf(stderr, "\t-n NUM, --preset_num NUM\n");
    fprintf(stderr, "\t\tset preset NUM (0-14) when using GO_PRESET, SET_PRESET or DEL_PRESET actions\n");
    fprintf(stderr, "\t\tspecial presets: 10 center\n");
    fprintf(stderr, "\t\t                 11 upper left corner\n");
    fprintf(stderr, "\t\t                 12 upper right corner\n");
    fprintf(stderr, "\t\t                 13 lower right corner\n");
    fprintf(stderr, "\t\t                 14 lower left corner\n");
    fprintf(stderr, "\t-e DESCRIPTION, --desc DESCRIPTION\n");
    fprintf(stderr, "\t\tset description (used with SET_PRESET and SET_HOME action)\n");
    fprintf(stderr, "\t-f PRESET_FILE, --file PRESET_FILE\n");
    fprintf(stderr, "\t\tset preset configuration file (GET_PRESETS, GO_PRESET and SET_PRESET, DEL_PRESET, SET_HOME  actions)\n");
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
        fprintf(stderr, "Error reading configuration file\n");
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
                fprintf(stderr, "Error reading configuration file\n");
                exit(EXIT_FAILURE);
            }
            p = strtok(NULL, "|");
            if (p != NULL) {
                errno = 0;
                nvalue = strtol(p, NULL, 10);
                if (errno == 0) {
                    presets[ikey].y = nvalue;
                } else {
                    fprintf(stderr, "Error reading configuration file\n");
                    exit(EXIT_FAILURE);
                }
                p = strtok(NULL, "|");
                if (p != NULL) {
                    fprintf(stderr, "Error reading configuration file\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                fprintf(stderr, "Error reading configuration file\n");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Error reading configuration file\n");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Error reading configuration file\n");
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
    double X, Y;
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
    time = -1;
    x = -1;
    y = -1;
    X = -1.0;
    Y = -1.0;
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
            {"X",  required_argument, 0, 'X'},
            {"Y",  required_argument, 0, 'Y'},
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

        c = getopt_long (argc, argv, "a:t:x:y:X:Y:f:n:e:cdh",
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
            } else if (strcasecmp("go_rel", optarg) == 0) {
                action = ACTION_GO_REL;
            } else if (strcasecmp("get_presets", optarg) == 0) {
                action = ACTION_GET_PRESETS;
            } else if (strcasecmp("set_preset", optarg) == 0) {
                action = ACTION_SET_PRESET;
            } else if (strcasecmp("del_preset", optarg) == 0) {
                action = ACTION_DEL_PRESET;
            } else if (strcasecmp("set_home", optarg) == 0) {
                action = ACTION_SET_HOME;
            } else if (strcasecmp("go_preset", optarg) == 0) {
                action = ACTION_GO_PRESET;
            } else if (strcasecmp("get_coord", optarg) == 0) {
                action = ACTION_GET_COORD;
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
            if ((time < 0) || (time > 10000)) {
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
            if ((x < -MAX_X) || (x > MAX_X)) {
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
            if ((y < -MAX_Y) || (y > MAX_Y)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'X':
            errno = 0;    /* To distinguish success/failure after call */
            X = strtod(optarg, &endptr);

            /* Check for various possible errors */
            if (errno == ERANGE) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if (endptr == optarg) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if ((X < -MAX_X_DEG) || (X > MAX_X_DEG)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'Y':
            errno = 0;    /* To distinguish success/failure after call */
            Y = strtod(optarg, &endptr);

            /* Check for various possible errors */
            if (errno == ERANGE) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if (endptr == optarg) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if ((Y < -MAX_Y_DEG) || (Y > MAX_Y_DEG)) {
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
            if ((preset_num < -1) || (preset_num > 14)) {
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
    if ((action == ACTION_SET_PRESET) && (preset_num > 9)) {
        fprintf(stderr, "preset_num must be between 0 and 9.\n");
        print_usage(argv[0]);
        return -1;
    }
    if (((action == ACTION_SET_PRESET) || (action == ACTION_GO_PRESET) ||
            (action == ACTION_DEL_PRESET) || (action == ACTION_SET_HOME) ||
            (action == ACTION_GET_PRESETS)) &&
            (preset_file[0] == '\0')) {
        fprintf(stderr, "preset_file cannot be empty.\n");
        print_usage(argv[0]);
        return -1;
    }
    if (((action == ACTION_SET_PRESET) || (action == ACTION_SET_HOME)) &&
            (desc[0] == '\0')) {
        fprintf(stderr, "desc cannot be empty.\n");
        print_usage(argv[0]);
        return -1;
    }
    if ((clear == 1) && (action != ACTION_SET_PRESET)) {
        fprintf(stderr, "clear flag must be used with set_preset action.\n");
        print_usage(argv[0]);
        return -1;
    }
    if ((clear == 1) && (action == ACTION_SET_PRESET) && (preset_num == -1)) {
        fprintf(stderr, "clear flag must be used with a valid preset number.\n");
        print_usage(argv[0]);
        return -1;
    }
    if ((action == ACTION_DEL_PRESET) && (preset_num == -1)) {
        fprintf(stderr, "preset_num must be specified.\n");
        print_usage(argv[0]);
        return -1;
    }
    // Check coordinates when passed as internal number
    if ((action == ACTION_GO) && (x != -1) && (y != -1)) {
        if ((x < MIN_X) || (x > MAX_X)) {
            fprintf(stderr, "value x is out of range.\n");
            print_usage(argv[0]);
            return -1;
        }
        if ((action == ACTION_GO) && ((y < MIN_Y) || (y > MAX_Y))) {
            fprintf(stderr, "value y is out of range.\n");
            print_usage(argv[0]);
            return -1;
        }
    }
    // End of arguments checks

    if (action == ACTION_GET_COORD) {
        double fx, fy;
        memset(preset_buffer, '\0', sizeof(preset_buffer));
        if (hw_ptz_pos_read(0, preset_buffer, 0, 0) != 0) {
            fprintf(stderr, "Error reading position\n");
            return -2;
        }
        x = preset_buffer[3];
        y = preset_buffer[4];

        if (debug) {
            fprintf(stderr, "PTZ_POS_READ:\n");
            for (i = 0; i < sizeof(preset_buffer) / sizeof(int); i++) {
                fprintf(stderr, "%08x\n", preset_buffer[i]);
            }
        }
        fprintf(stderr, "Current x  = %d, y  = %d\n", x, y);
        fx = ((double) x) / ((double) MAX_X) * MAX_X_DEG;
        fy = ((double) y) / ((double) MAX_Y) * MAX_Y_DEG;
        // Invert x otherwise 0 is right and 360 is left
        fx = MAX_X_DEG - fx;

        fprintf(stderr, "Degrees x  = %.1f, y  = %.1f\n", fx, fy);
        fprintf(stdout, "%.1f,%.1f\n", fx, fy);
        return 0;
    }

    if (debug) fprintf(stderr, "Running action %d\n", action);

    if (action == ACTION_GET_PRESETS) {
        double fx, fy;

        // Load presets from config file
        ptz_init_config(preset_file);
        for (i = 0; i < PRESET_NUM; i++) {
            fx = ((double) presets[i].x) * MAX_X_DEG / ((double) MAX_X);
            fy = ((double) presets[i].y) * MAX_Y_DEG / ((double) MAX_Y);
            if (strcmp("empty", presets[i].desc) == 0) {
                fprintf(stdout, "%d=,,\n", i);
            } else {
                fprintf(stdout, "%d=%s,%.1f,%.1f\n", i, presets[i].desc, fx, fy);
            }
        }
    }

    if (action == ACTION_SET_PRESET) {
        // Load presets from config file
        ptz_init_config(preset_file);

        // If no preset number is selected, check for free preset
        if (preset_num == -1) {
            for (i=0; i<10; i++) {
                if (strcasecmp("empty", presets[i].desc) == 0) {
                    preset_num = i;
                    break;
                }
            }
            if (preset_num == -1) {
                fprintf(stderr, "No preset available\n");
                printf("%d\n", preset_num);
                return 0;
            }
        }

        memset(preset_buffer, '\0', sizeof(preset_buffer));
        if (clear == 1) {
            x = -1;
            y = -1;
            strcpy(desc, "empty");
        } else {
            if (hw_ptz_pos_read(0, preset_buffer, 0, 0) != 0) {
                fprintf(stderr, "Error reading position\n");
                return -2;
            }
            if (x < MIN_X) x = MIN_X;
            if (x > MAX_X) x = MAX_X;
            if (y < MIN_Y) y = MIN_Y;
            if (y < MAX_Y) y = MAX_Y;
            x = preset_buffer[3];
            y = preset_buffer[4];
            if (debug) fprintf(stderr, "Current position: (%d, %d)\n", x, y);
        }

        presets[preset_num].x = x;
        presets[preset_num].y = y;
        strcpy(presets[preset_num].desc, desc);
        ptz_save_config(preset_file);

        // Print the number of preset
        if (clear != 1) {
            printf("%d\n", preset_num);
        }

        return 0;
    }

    if (action == ACTION_DEL_PRESET) {
        // Load presets from config file
        ptz_init_config(preset_file);

        presets[preset_num].x = -1;
        presets[preset_num].y = -1;
        strcpy(presets[preset_num].desc, "empty");
        ptz_save_config(preset_file);

        // Print the number of preset
        if (clear != 1) {
            printf("%d\n", preset_num);
        }

        return 0;
    }

    if (action == ACTION_SET_HOME) {
        // Load presets from config file
        ptz_init_config(preset_file);

        memset(preset_buffer, '\0', sizeof(preset_buffer));
        if (hw_ptz_pos_read(0, preset_buffer, 0, 0) != 0) {
            fprintf(stderr, "Error reading position\n");
            return -2;
        }
        if (x < MIN_X) x = MIN_X;
        if (x > MAX_X) x = MAX_X;
        if (y < MIN_Y) y = MIN_Y;
        if (y < MAX_Y) y = MAX_Y;
        x = preset_buffer[3];
        y = preset_buffer[4];
        if (debug) fprintf(stderr, "Current position: (%d, %d)\n", x, y);

        presets[0].x = x;
        presets[0].y = y;
        strcpy(presets[0].desc, desc);
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
        if ((x < 0) || (y < 0)) {
            if (debug) fprintf(stderr, "Invalid preset: %d\n", preset_num);
            return 0;
        } else {
            if (debug) fprintf(stderr, "Go to preset %d: (%d, %d)\n", preset_num, presets[0].x, presets[0].y);
        }
    }

    if ((action == ACTION_GO) && (x != -1) && (y != -1)) {
        // Do nothing
    }

    if ((action == ACTION_GO) && (X != -1.0) && (Y != -1.0)) {
        double fx, fy;
        int cur_x, cur_y;

        memset(preset_buffer, '\0', sizeof(preset_buffer));
        if (hw_ptz_pos_read(0, preset_buffer, 0, 0) != 0) {
            fprintf(stderr, "Error reading position\n");
            return -2;
        }
        cur_x = preset_buffer[3];
        cur_y = preset_buffer[4];

        fx = X * (((double) MAX_X) / MAX_X_DEG);
        x = round(fx);
        // Invert x otherwise 0 is right and 360 is left
        x = MAX_X - x;
        if (x > MAX_X) x = MAX_X;
        if (x < MIN_X) x = MIN_X;
        fy = Y * (((double) MAX_Y) / MAX_Y_DEG);
        y = round(fy);
        if (y > MAX_Y) y = MAX_Y;
        if (y < MIN_Y) y = MIN_Y;

        if (debug) fprintf(stderr, "Current x  = %d, y  = %d\n", cur_x, cur_y);
        if (debug) fprintf(stderr, "Command xr = %d, yr = %d\n", x, y);
    }

    if ((action == ACTION_GO_REL) && (x != -1) && (y != -1)) {
        int cur_x, cur_y;
        int fin_x, fin_y;

        memset(preset_buffer, '\0', sizeof(preset_buffer));
        if (hw_ptz_pos_read(0, preset_buffer, 0, 0) != 0) {
            fprintf(stderr, "Error reading position\n");
            return -2;
        }
        cur_x = preset_buffer[3];
        cur_y = preset_buffer[4];

        fin_x = cur_x + x;
        if (fin_x > MAX_X) fin_x = MAX_X;
        if (fin_x < MIN_X) fin_x = MIN_X;
        fin_y = cur_y + y;
        if (fin_y > MAX_Y) fin_y = MAX_Y;
        if (fin_y < MIN_Y) fin_y = MIN_Y;
        x = fin_x;
        y = fin_y;

        if (debug) fprintf(stderr, "Current x  = %d, y  = %d\n", cur_x, cur_y);
        if (debug) fprintf(stderr, "Command xr = %d, yr = %d\n", x, y);
    }

    if ((action == ACTION_GO_REL) && (X != -1) && (Y != -1)) {
        double fx, fy;
        int cur_x, cur_y;
        int dx, dy;
        int fin_x, fin_y;

        memset(preset_buffer, '\0', sizeof(preset_buffer));
        if (hw_ptz_pos_read(0, preset_buffer, 0, 0) != 0) {
            fprintf(stderr, "Error reading position\n");
            return -2;
        }
        cur_x = preset_buffer[3];
        cur_y = preset_buffer[4];

        fx = X * (((double) MAX_X) / MAX_X_DEG);
        dx = round(fx);
        // Invert x otherwise 0 is right and 360 is left
        dx = -dx;
        fy = Y * (((double) MAX_Y) / MAX_Y_DEG);
        dy = round(fy);

        fin_x = cur_x + dx;
        if (fin_x > MAX_X) fin_x = MAX_X;
        if (fin_x < MIN_X) fin_x = MIN_X;
        fin_y = cur_y + dy;
        if (fin_y > MAX_Y) fin_y = MAX_Y;
        if (fin_y < MIN_Y) fin_y = MIN_Y;
        x = fin_x;
        y = fin_y;

        if (debug) fprintf(stderr, "Current x  = %d, y  = %d\n", cur_x, cur_y);
        if (debug) fprintf(stderr, "Command xr = %d, yr = %d\n", x, y);
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


    if (time != -1) {
        // Send stop command
        if ((action == ACTION_RIGHT) || (action == ACTION_LEFT) ||
                 (action == ACTION_DOWN) || (action == ACTION_UP)) {

            usleep(time * 1000);

            if (debug) fprintf(stderr, "Running stop\n");
            ptz_arg[0] = ACTION_STOP;
            hw_ptz_sendptz(ptz_arg);
        }
    }

    return 0;
}
