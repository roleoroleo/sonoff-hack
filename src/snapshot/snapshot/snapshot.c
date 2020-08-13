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
 * Create a jpg snapshot.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "snapshot.h"

void print_usage(char *progname)
{
    fprintf(stderr, "\nUsage: %s [-f filename [-d] [-h]\n\n", progname);
    fprintf(stderr, "\t-f filename, --file filename\n");
    fprintf(stderr, "\t\tsave jpg to filename\n");
    fprintf(stderr, "\t-d,     --debug\n");
    fprintf(stderr, "\t\tenable debug\n");
    fprintf(stderr, "\t-h,     --help\n");
    fprintf(stderr, "\t\tprint this help\n");
}

int main(int argc, char **argv)
{
    char filename[1024];
    int debug;
    int c;
    int errno;
    char *endptr;
    int stdout_output;

    int sockfd;
    struct sockaddr_in local_addr, avencode_addr;
    char *message;

    FILE *fp;
    char buffer[4096];
    int n_recv;
    size_t n_read;

    // Setting default
    tmpnam(filename);
    stdout_output = 1;
    debug = 0;

    while (1) {
        static struct option long_options[] =
        {
            {"filename",  required_argument, 0, 'f'},
            {"debug",  no_argument, 0, 'd'},
            {"help",  no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "f:dh",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {

        case 'f':
            memset(filename, '\0', 1024);
            if (strlen(optarg) >= 1024)
                strncpy(filename, optarg, 1024);
            else
                strncpy(filename, optarg, strlen(optarg));
            stdout_output = 0;
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

    if (debug) fprintf(stderr, "Starting program\n");

    if (debug) fprintf(stderr, "File %s selected\n", filename);

    // Create UDP socket
    sockfd = socket(AF_INET , SOCK_DGRAM , 0);
    if (sockfd == -1) {
        fprintf(stderr, "Could not create socket\n");
        return -1;
    }

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(LOCAL_IP);
    local_addr.sin_port = htons(0);

    avencode_addr.sin_family = AF_INET;
    avencode_addr.sin_addr.s_addr = inet_addr(AVENCODE_IP);
    avencode_addr.sin_port = htons(AVENCODE_PORT);

    // Bind the socket with the local address
    if (bind(sockfd, (const struct sockaddr *) &local_addr, sizeof(local_addr)) < 0) {
        fprintf(stderr, "Bind failed\n");
        return -2;
    }

    if (debug) fprintf(stderr, "Socket created\n");

    // Build message
    message = (char *) malloc(MSG_SIZE * sizeof(char));
    memset(message, '\0', MSG_SIZE);
    memcpy(message, MSG_PART_1, sizeof(MSG_PART_1) - 1);
    memcpy(message + sizeof(MSG_PART_1) - 1, filename, strlen(filename));
    memcpy(message + MSG_SIZE - 4, MSG_PART_3, sizeof(MSG_PART_3) - 1);

    // Send data
    if (debug) fprintf(stderr, "Sending message to %s port %d\n", AVENCODE_IP, AVENCODE_PORT);
    if (sendto(sockfd, message, MSG_SIZE, 0,
                (const struct sockaddr *) &avencode_addr, sizeof(avencode_addr)) < 0) {
        fprintf(stderr, "Send failed");
        return -1;
    }
    if (debug) fprintf(stderr, "Data successfully sent\n");

    n_recv = recv(sockfd, (char *)buffer, 1024, 0);
    if (debug) fprintf(stderr, "Confirmation received (%d bytes packet)\n", n_recv);

    sleep(1);

    // Read file and send to stdout
    if (stdout_output == 1) {
        if (debug) fprintf(stderr, "Sending file to stdout\n");
        fp = fopen(filename, "r");
        if (fp == NULL)
            return -1;

        while ((n_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
            fwrite(buffer, 1, n_read, stdout);

        fclose(fp);
        remove(filename);
    }
    if (debug) fprintf(stderr, "Program completed successfully\n");

    return 0;
}
