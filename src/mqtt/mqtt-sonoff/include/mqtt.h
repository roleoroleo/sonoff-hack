#ifndef MQTT_H
#define MQTT_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <mosquitto.h>

#define EMPTY_TOPIC         ""

typedef struct
{
    char        client_id[64];
    char       *user;
    char       *password;
    char        host[128];
    int         port;
    int         keepalive;
    char        bind_address[32];
    int         qos;
    int         retain_birth_will;
    int         retain_motion;
    int         retain_motion_image;
    int         ipcsys_db;

    char       *mqtt_prefix;
    char       *topic_birth_will;
    char       *birth_msg;
    char       *will_msg;
} mqtt_conf_t;

typedef struct
{
    char* topic;
    char* msg;
    int len;
} mqtt_msg_t;

int init_mqtt(void);
void stop_mqtt(void);

void mqtt_loop(void);

void mqtt_init_conf(mqtt_conf_t *conf);
void mqtt_set_conf(mqtt_conf_t *conf);
void mqtt_check_connection();
int mqtt_send_message(mqtt_msg_t *msg, int retain);

int mqtt_connect();

#endif // MQTT_H
