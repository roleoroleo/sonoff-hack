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

/*
 * Send message to a IPC system.
 */

#include <getopt.h>
#include <sys/stat.h>
#include "udp.h"
#include "ipc_cmd.h"

extern unsigned char PKT_ROTATE_ON[];
extern unsigned char PKT_ROTATE_OFF[];
extern unsigned char PKT_MD_OFF[];
extern unsigned char PKT_MD_25[];
extern unsigned char PKT_MD_50[];
extern unsigned char PKT_MD_75[];
extern unsigned char PKT_LOCAL_RECORD_ON[];
extern unsigned char PKT_LOCAL_RECORD_OFF[];

int debug;

void print_usage(char *progname)
{
    fprintf(stderr, "\nUsage: %s [t ON/OFF] [-m MOTION] [-s SENS] [-l LOCAL] [-i IR] [-r ROTATE] [-d] [-h]\n\n", progname);
    fprintf(stderr, "\t-t ON/OFF, --switch ON/OFF\n");
    fprintf(stderr, "\t\tswitch ON or OFF the cam\n");
    fprintf(stderr, "\t-m MOTION, --motion_detection MOTION\n");
    fprintf(stderr, "\t\tenable/disable motion detection: ON or OFF\n");
    fprintf(stderr, "\t-s SENS, --sensitivity SENS\n");
    fprintf(stderr, "\t\tset sensitivity: LOW, MEDIUM or HIGH\n");
    fprintf(stderr, "\t-l LOCAL, --local_record LOCAL\n");
    fprintf(stderr, "\t\tenable/disable local record: ON or OFF\n");
    fprintf(stderr, "\t-i IR, --ir IR\n");
    fprintf(stderr, "\t\tset ir led: AUTO, ON or OFF\n");
    fprintf(stderr, "\t-r ROTATE, --rotate ROTATE\n");
    fprintf(stderr, "\t\tset rotate: ON or OFF\n");
    fprintf(stderr, "\t-d, --debug\n");
    fprintf(stderr, "\t\tenable debug\n");
    fprintf(stderr, "\t-h, --help\n");
    fprintf(stderr, "\t\tprint this help\n");
}

int main(int argc, char ** argv)
{
    int c;
    int switch_on = NONE;
    int motion = NONE;
    int sensitivity = NONE;
    int local = NONE;
    int ir = NONE;
    int rotate = NONE;
    char cmd_line[1024];
    int touch_fd;

    cmd_line[0] = '\0';
    debug = 0;

    while (1) {
        static struct option long_options[] =
        {
            {"switch",  required_argument, 0, 't'},
            {"motion",  required_argument, 0, 'm'},
            {"sensitivity",  required_argument, 0, 's'},
            {"local",  required_argument, 0, 'l'},
            {"ir",  required_argument, 0, 'i'},
            {"rotate",  required_argument, 0, 'r'},
            {"debug",  no_argument, 0, 'd'},
            {"help",  no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "t:m:s:l:i:r:dh",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
        case 't':
            if ((strcasecmp("off", optarg) == 0) || (strcasecmp("no", optarg) == 0)) {
                switch_on = SWITCH_OFF;
            } else if ((strcasecmp("on", optarg) == 0) || (strcasecmp("yes", optarg) == 0)) {
                switch_on = SWITCH_ON;
            } else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'm':
            if ((strcasecmp("off", optarg) == 0) || (strcasecmp("no", optarg) == 0)) {
                motion = MOTION_OFF;
            } else if ((strcasecmp("on", optarg) == 0) || (strcasecmp("yes", optarg) == 0)) {
                motion = MOTION_ON;
            } else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 's':
            if (strcasecmp("low", optarg) == 0) {
                sensitivity = SENSITIVITY_LOW;
            } else if (strcasecmp("medium", optarg) == 0) {
                sensitivity = SENSITIVITY_MEDIUM;
            } else if (strcasecmp("high", optarg) == 0) {
                sensitivity = SENSITIVITY_HIGH;
            } else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'l':
            if ((strcasecmp("off", optarg) == 0) || (strcasecmp("no", optarg) == 0)) {
                local = LOCAL_OFF;
            } else if ((strcasecmp("on", optarg) == 0) || (strcasecmp("yes", optarg) == 0)) {
                local = LOCAL_ON;
            } else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'i':
            if ((strcasecmp("off", optarg) == 0) || (strcasecmp("no", optarg) == 0)) {
                ir = IR_OFF;
            } else if ((strcasecmp("on", optarg) == 0) || (strcasecmp("yes", optarg) == 0)) {
                ir = IR_ON;
            } else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'r':
            if ((strcasecmp("off", optarg) == 0) || (strcasecmp("no", optarg) == 0)) {
                rotate = ROTATE_OFF;
            } else if ((strcasecmp("on", optarg) == 0) || (strcasecmp("yes", optarg) == 0)) {
                rotate = ROTATE_ON;
            } else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'd':
            fprintf (stderr, "debug on\n");
            debug = 1;
            break;

        case 'h':
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
            break;

        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        }
    }

    if (argc == 1) {
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    if (switch_on == SWITCH_ON) {
        sprintf(cmd_line, "%s off", PRIVACY_SCRIPT);
        system(cmd_line);
    } else if (switch_on == SWITCH_OFF) {
        sprintf(cmd_line, "%s on", PRIVACY_SCRIPT);
        system(cmd_line);
    }

    if (motion == MOTION_ON) {
        udp_send(LOCALHOST, ALARMSERVER_PORT, PKT_MD_25, 120);
    } else if (motion == MOTION_OFF) {
        udp_send(LOCALHOST, ALARMSERVER_PORT, PKT_MD_OFF, 120);
    }

    if (sensitivity == SENSITIVITY_LOW) {
        udp_send(LOCALHOST, ALARMSERVER_PORT, PKT_MD_25, 120);
    } else if (sensitivity == SENSITIVITY_MEDIUM) {
        udp_send(LOCALHOST, ALARMSERVER_PORT, PKT_MD_50, 120);
    } else if (sensitivity == SENSITIVITY_HIGH) {
        udp_send(LOCALHOST, ALARMSERVER_PORT, PKT_MD_75, 120);
    }

    if (local == LOCAL_ON) {
        udp_send(LOCALHOST, AVRECSCH_PORT, PKT_LOCAL_RECORD_ON, 1184);
    } else if (local == LOCAL_OFF) {
        udp_send(LOCALHOST, AVRECSCH_PORT, PKT_LOCAL_RECORD_OFF, 1184);
    }

    if (ir != NONE) {
        if (ir == IR_AUTO) {
            strcpy(cmd_line, IR_AUTO_FILE);
        } else if (ir == IR_ON) {
            strcpy(cmd_line, IR_DARK_FILE);
        } else if (ir == IR_OFF) {
            strcpy(cmd_line, IR_BRIGHT_FILE);
        }

        touch_fd = open(cmd_line, O_CREAT | S_IRUSR | S_IWUSR);
        if (touch_fd == -1) {
            fprintf(stderr, "Unable to touch file %s\n", cmd_line);
        } else {
            close(touch_fd);
        }
    }

    if (rotate == ROTATE_ON) {
        udp_send(LOCALHOST, DEVCTRL_PORT, PKT_ROTATE_ON, 80);
    } else if (rotate == ROTATE_OFF) {
        udp_send(LOCALHOST, DEVCTRL_PORT, PKT_ROTATE_OFF, 80);
    }

    return 0;
}
