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
#include "libptz.h"

void print_usage(char *progname)
{
    fprintf(stderr, "\nUsage: %s [i DIR] [-d] [-h]\n\n", progname);
    fprintf(stderr, "\t-a ACTION, --action ACTION\n");
    fprintf(stderr, "\t\tset PTZ action: stop, right, left, down, up\n");
    fprintf(stderr, "\t-d,     --debug\n");
    fprintf(stderr, "\t\tenable debug\n");
    fprintf(stderr, "\t-h,     --help\n");
    fprintf(stderr, "\t\tprint this help\n");
}

int main(int argc, char **argv)
{
    int action;
    int debug;
    int c;
    int errno;
    char *endptr;
    int ptz_arg[8];

    // Setting default
    action = ACTION_STOP;
    debug = 0;

    while (1) {
        static struct option long_options[] =
        {
            {"action",  required_argument, 0, 'a'},
            {"debug",  no_argument, 0, 'd'},
            {"help",  no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "a:dh",
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
            }
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

    if (debug) fprintf(stderr, "running action %d\n", action);
    ptz_arg[0] = action;
    ptz_arg[1] = 0;
    ptz_arg[2] = 1; // 1, 2 or 3
    ptz_arg[3] = 0; // absolute position x
    ptz_arg[4] = 0; // absolute position y
    ptz_arg[5] = 0; // ptz crz
    ptz_arg[6] = 0;
    ptz_arg[7] = 0;
    hw_ptz_sendptz(ptz_arg);

    usleep(ACTION_TIME * 1000);

    if (debug) fprintf(stderr, "running stop\n");

    ptz_arg[0] = ACTION_STOP;
    hw_ptz_sendptz(ptz_arg);

    return 0;
}
