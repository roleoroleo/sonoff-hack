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

#ifndef MQTT_SONOFF_H
#define MQTT_SONOFF_H

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h>
#include <limits.h>

#include "config.h"
#include "sql.h"
#include "mqtt.h"
#include "inotify.h"
#include "cJSON.h"

#define MQTT_SONOFF_VERSION      "0.1.0"
#define MQTT_SONOFF_CONF_FILE    "/mnt/mmc/sonoff-hack/etc/mqtt-sonoff.conf"
#define COLINK_CONF_FILE         "/mnt/mtd/ipc/cfg/colink.conf"
#define HACK_VERSION_FILE        "/mnt/mmc/sonoff-hack/version"
#define HACK_MODEL_FILE          "/mnt/mmc/sonoff-hack/model"

#define MQTT_SONOFF_SNAPSHOT     "/mnt/mmc/sonoff-hack/bin/snapshot"
#define CONF2MQTT_SCRIPT         "/mnt/mmc/sonoff-hack/script/conf2mqtt.sh"
#define IPC_CMD                  "/mnt/mmc/sonoff-hack/bin/ipc_cmd"
#define PTZ_CMD                  "/mnt/mmc/sonoff-hack/bin/ptz"


#define MOTION_ALARM_FILE        "/tmp/onvif_notify_server/motion_alarm"

typedef struct
{
    char    *mqtt_prefix;
    char    *mqtt_prefix_stat;
    char    *topic_birth_will;
    char    *topic_motion;
    char    *topic_motion_image;
    double   motion_image_delay;
    char    *birth_msg;
    char    *will_msg;
    char    *motion_start_msg;
    char    *motion_stop_msg;
    int      ha_enable_discovery;
    char    *ha_conf_prefix;
    char    *ha_name_prefix;
    char    *device_id;
    char    *device_model;
    char    *fw_version;
} mqtt_sonoff_conf_t;

#endif // MQTT_SONOFF_H
