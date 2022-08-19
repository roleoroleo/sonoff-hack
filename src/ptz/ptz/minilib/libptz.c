/*
 * Copyright (c) 2022 puuu.
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
 * minimal implementation of libptz.so to control PTZ via /dev/ptz
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "libptz.h"

#define DEVICE "/dev/ptz"


int hw_ptz_sendptz(int *ptz_arg) {
    int error = 0;
    int fd_driver = open(DEVICE, O_RDWR);
    if (fd_driver == -1) {
        printf("ERROR: could not open \"%s\".\n", DEVICE);
        printf("    errno = %s\n", strerror(errno));
        return -1;
    }
    unsigned int value=0x1;
    if (ioctl(fd_driver, 0xa, &value) < 0) {
        printf("Error ioctl 0xa\n");
        printf("    errno = %s\n", strerror(errno));
        error = 1;
    }
    if ((ptz_arg[2] > 1) && (ptz_arg[2] < 4)) {
        value = 5 - ptz_arg[2];
	    if (!error && ioctl(fd_driver, 0x9, &value) < 0) {
			printf("Error ioctl 0x9\n");
            printf("    errno = %s\n", strerror(errno));
			error = 1;
	    }
    }
    if ((ptz_arg[0] > 0) && (ptz_arg[0] < 5)) {
        int action = 4 + ptz_arg[0];
        value=0x1388;
	    if (!error && ioctl(fd_driver, action, &value) < 0) {
			printf("Error ioctl action = 0x%x\n", action);
            printf("    errno = %s\n", strerror(errno));
			error = 1;
	    }
    }
    if (ptz_arg[0] == 14) {
        int data[10] = {};
        data[3] = ptz_arg[3];  // x
        data[4] = ptz_arg[4];  // y
        data[8] = 1;  // y
        if (!error && write(fd_driver, (char *)data, sizeof(data) * sizeof(int)) < 0) {
			printf("Error write()\n");
            printf("    errno = %s\n", strerror(errno));
			error = 1;
        }
    }
    close(fd_driver);
    if (error) {
        return -1;
    }
    return 0;
}

int hw_ptz_pos_read(int arg1, int *buffer, int arg3, int arg4) {
    int error = 0;
    int fd_driver = open(DEVICE, O_RDWR);
    if (fd_driver == -1) {
        printf("ERROR: could not open \"%s\".\n", DEVICE);
        printf("    errno = %s\n", strerror(errno));
        return -1;
    }
    if (read(fd_driver, (char *)buffer, 40) < 0) {
        printf("Error read()\n");
        printf("    errno = %s\n", strerror(errno));
        error = 1;
    }
    close(fd_driver);
    if (error) {
        return -1;
    }
    return 0;
}
