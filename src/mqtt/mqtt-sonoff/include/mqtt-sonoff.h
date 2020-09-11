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

#define MQTT_SONOFF_VERSION      "0.0.1"
#define MQTT_SONOFF_CONF_FILE    "/mnt/mmc/sonoff-hack/etc/mqtt-sonoff.conf"

typedef struct
{
    char *mqtt_prefix;
    char *topic_birth_will;
    char *topic_motion;
    char *birth_msg;
    char *will_msg;
    char *motion_start_msg;
} mqtt_sonoff_conf_t;

#endif // MQTT_SONOFF_H
