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

#include "inotify.h"

static pthread_t *tr_inotify;
static int tr_inotify_routine;

static int start_inotify_thread();
static void *inotify_thread(void *arg);

static void call_callback(INOTIFY_MESSAGE_TYPE type);
static void inotify_debug(const char* fmt, ...);

extern int debug;

//-----------------------------------------------------------------------------
// MESSAGES HANDLERS
//-----------------------------------------------------------------------------

static void handle_inotify_motion_start();

//-----------------------------------------------------------------------------
// FUNCTION POINTERS TO CALLBACKS
//-----------------------------------------------------------------------------

typedef void(*func_ptr_t)(void* arg);

static func_ptr_t *inotify_callbacks;

/* Read all available inotify events from the file descriptor 'fd'.
   wd is the table of watch descriptors for the directories in argv.
   argc is the length of wd and argv.
   argv is the list of watched directories.
   Entry 0 of wd and argv is unused. */

static void handle_events(int fd)
{
    /* Some systems cannot read integer variables if they are not
       properly aligned. On other systems, incorrect alignment may
       decrease performance. Hence, the buffer used for reading from
       the inotify file descriptor should have the same alignment as
       struct inotify_event. */

    char buf[4096]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    ssize_t len;
    char *ptr;

    /* Loop while events can be read from inotify file descriptor. */

    for (;;) {

        /* Read some events. */

        len = read(fd, buf, sizeof(buf));
        if (len == -1 && errno != EAGAIN) {
            fprintf(stderr, "Error reading from file descriptor\n");
            return;
        }

        /* If the nonblocking read() found no events to read, then
           it returns -1 with errno set to EAGAIN. In that case,
           we exit the loop. */

        if (len <= 0)
            break;

        /* Loop over all events in the buffer. */

        for (ptr = buf; ptr < buf + len;
                ptr += sizeof(struct inotify_event) + event->len) {

            event = (const struct inotify_event *) ptr;

            /* Handle "File created", "Metadata change, e.g. Timestamp" and "File modified" evets */
            if (event->mask & (IN_CREATE|IN_MODIFY|IN_ATTRIB)) {
                /* Don't handle directories */
                if ((event->mask & IN_ISDIR) != IN_ISDIR) {
                    /* Filename (event->name) length is gt 0 */
                    if (event->len) {
                        /* Is it the file to watch? */
                        if (strcmp(NOTI_FILE, event->name) == 0) {
                            if (debug) fprintf(stderr, "inotify: evt %08x on %s\n", event->mask, event->name);
                            handle_inotify_motion_start();
                        }
                    }
                }
            }
        }
    }
}

static void *inotify_thread(void *arg)
{
    int fd, poll_num;
    int wd;
    struct pollfd fds[2];

    /* Create the file descriptor for accessing the inotify API. */
    fd = inotify_init1(IN_NONBLOCK);
    if (fd == -1) {
        fprintf(stderr, "Error in inotify_init1\n");
        return NULL;
    }

    /* Add watch to the directory. */
    wd = inotify_add_watch(fd, TMP_DIR, IN_CREATE|IN_MODIFY|IN_ATTRIB);
    if (wd == -1) {
        fprintf(stderr, "Cannot watch '%s': %s\n", TMP_DIR, strerror(errno));
        return NULL;
    }

    /* Prepare for polling. */
    fds[0].fd = fd;
    fds[0].events = POLLIN;

    /* Wait for events and/or terminal input. */
    if (debug) fprintf(stderr, "Listening for create events in %s\n", TMP_DIR);
    while (1) {
        if (tr_inotify_routine == 0) break;

        poll_num = poll(fds, 1, -1);
        if (poll_num == -1) {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "Error in poll\n");
        }

        if (poll_num > 0) {
            if (fds[0].revents & POLLIN) {

                /* Inotify events are available. */
                handle_events(fd);
            }
        }
    }

    if (debug) fprintf(stderr, "Listening for events stopped.\n");

    /* Close inotify file descriptor. */
    close(fd);

    return NULL;
}

int start_inotify_thread()
{
    int ret;

    tr_inotify = malloc(sizeof(pthread_t));
    tr_inotify_routine = 1;
    ret = pthread_create(tr_inotify, NULL, &inotify_thread, NULL);
    if(ret != 0)
    {
        fprintf(stderr, "Can't create inotify thread. Error: %d\n", ret);
        return -1;
    }

    return 0;
}

int inotify_start()
{
    int ret = 0;

    tr_inotify = NULL;
    inotify_callbacks = NULL;

    ret = start_inotify_thread();
    if(ret != 0)
        return -1;

    inotify_callbacks = malloc((sizeof(func_ptr_t)) * INOTIFY_MSG_LAST);

    return 0;
}

void inotify_stop()
{
    if(tr_inotify != NULL)
    {
        tr_inotify_routine = 0;
        pthread_join(*tr_inotify, NULL);
        free(tr_inotify);
    }

    if(inotify_callbacks != NULL)
        free(inotify_callbacks);
}

//-----------------------------------------------------------------------------
// GETTERS AND SETTERS
//-----------------------------------------------------------------------------

int inotify_set_callback(INOTIFY_MESSAGE_TYPE type, void (*f)())
{
    if(type >= INOTIFY_MSG_LAST)
        return -1;

    inotify_callbacks[(int)type]=f;

    return 0;
}

//-----------------------------------------------------------------------------
// HANDLERS
//-----------------------------------------------------------------------------

static void handle_inotify_motion_start()
{
    inotify_debug("GOT MOTION START\n");
    call_callback(INOTIFY_MSG_MOTION_START);
}

//-----------------------------------------------------------------------------
// UTILS
//-----------------------------------------------------------------------------

static void call_callback(INOTIFY_MESSAGE_TYPE type)
{
    func_ptr_t f;
    // Not handling callbacks with parameters (yet)
    f = inotify_callbacks[(int)type];
    if(f != NULL)
        (*f)(NULL);
}

static void inotify_debug(const char* fmt, ...)
{
    if (debug) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}
