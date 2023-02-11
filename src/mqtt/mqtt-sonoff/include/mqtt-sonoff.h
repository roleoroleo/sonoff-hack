#ifndef MQTT_SONOFF_H
#define MQTT_SONOFF_H

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "sql.h"
#include "mqtt.h"
#include "cJSON.h"

#define MQTT_SONOFF_VERSION      "0.1.0"
#define MQTT_SONOFF_CONF_FILE    "/mnt/mmc/sonoff-hack/etc/mqtt-sonoff.conf"
#define COLINK_CONF_FILE         "/mnt/mtd/ipc/cfg/colink.conf"
#define HACK_VERSION_FILE        "/mnt/mmc/sonoff-hack/version"

#define MQTT_SONOFF_SNAPSHOT     "/mnt/mmc/sonoff-hack/bin/snapshot"

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
