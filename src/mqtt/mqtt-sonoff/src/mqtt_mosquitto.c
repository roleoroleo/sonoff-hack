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

#ifdef USE_MOSQUITTO
#include "config.h"
#include "mqtt.h"
#include "mqtt-sonoff.h"

static struct mosquitto *mosq = NULL;
static mqtt_conf_t *mqtt_conf;

enum conn_states {CONN_DISCONNECTED, CONN_CONNECTING, CONN_CONNECTED};
static enum conn_states conn_state;
static int mid_sent;

static int init_mosquitto_instance();

static void connect_callback(struct mosquitto *mosq, void *obj, int result);
static void disconnect_callback(struct mosquitto *mosq, void *obj, int rc);
static void publish_callback(struct mosquitto *mosq, void *obj, int mid);
static void log_callback(struct mosquitto *mosq, void *obj, int level, const char *str);
static void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);

extern int debug;

void send_birth_msg()
{
    char topic[128];
    mqtt_msg_t msg;

    msg.msg = mqtt_conf->birth_msg;
    msg.len = strlen(msg.msg);
    msg.topic = topic;

    snprintf(topic, sizeof(topic), "%s/%s", mqtt_conf->mqtt_prefix, mqtt_conf->topic_birth_will);

    mqtt_send_message(&msg, mqtt_conf->retain_birth_will);
}

void send_will_msg()
{
    char topic[128];
    mqtt_msg_t msg;

    msg.msg = mqtt_conf->will_msg;
    msg.len = strlen(msg.msg);
    msg.topic = topic;

    snprintf(topic, sizeof(topic), "%s/%s", mqtt_conf->mqtt_prefix, mqtt_conf->topic_birth_will);

    mqtt_send_message(&msg, mqtt_conf->retain_birth_will);
}

int init_mqtt(void)
{
    int ret;

    ret = mosquitto_lib_init();
    if(ret != 0)
    {
        fprintf(stderr, "Can't initialize mosquitto library.\n");
        return -1;
    }

    ret = init_mosquitto_instance();
    if(ret != 0)
    {
        fprintf(stderr, "Can't create mosquitto instance.\n");
        return -2;
    }

    conn_state = CONN_DISCONNECTED;

    return 0;
}

void stop_mqtt(void)
{
    send_will_msg();
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}

void mqtt_loop(void)
{
    if(mosq != NULL)
        mosquitto_loop(mosq, -1, 1);
}

//-----------------------------------------------------------------------------

void mqtt_init_conf(mqtt_conf_t *conf)
{
    conf->user = NULL;
    conf->password = NULL;
    conf->client_id = NULL;

    strcpy(conf->host, "127.0.0.1");
    strcpy(conf->bind_address, "0.0.0.0");

    conf->port = 1883;
    conf->tls = 0;
    conf->keepalive = 120;
    conf->qos = 1;
    conf->retain_birth_will = 1;
    conf->retain_motion = 1;
    conf->retain_motion_image = 1;
    conf->ipcsys_db = 1;

    conf->mqtt_prefix = NULL;
    conf->topic_birth_will = NULL;
    conf->birth_msg = NULL;
    conf->will_msg = NULL;
}

void mqtt_set_conf(mqtt_conf_t *conf)
{
    mqtt_conf = conf;
}

void mqtt_check_connection()
{
    char topic[128];

    if (conn_state != CONN_CONNECTED) {
        snprintf(topic, sizeof(topic), "%s/%s", mqtt_conf->mqtt_prefix, mqtt_conf->topic_birth_will);
        mosquitto_will_set(mosq, topic, strlen(mqtt_conf->will_msg),
                    mqtt_conf->will_msg, mqtt_conf->qos, mqtt_conf->retain_birth_will == 1);
        mqtt_connect();
    }
}

int mqtt_connect()
{
    int ret;
    char topic[128];

    if (debug) fprintf(stderr, "Trying to connect... ");

    if(mqtt_conf->user != NULL && strcmp(mqtt_conf->user, "") != 0)
    {
        ret = mosquitto_username_pw_set(mosq, mqtt_conf->user, mqtt_conf->password);

        if(ret != MOSQ_ERR_SUCCESS)
        {
            fprintf(stderr, "Unable to set the auth parameters (%s).\n", mosquitto_strerror(ret));
            return -1;
        }
    }

    conn_state = CONN_DISCONNECTED;


    while (conn_state != CONN_CONNECTED) {
        snprintf(topic, sizeof(topic), "%s/%s", mqtt_conf->mqtt_prefix, mqtt_conf->topic_birth_will);
        mosquitto_will_set(mosq, topic, strlen(mqtt_conf->will_msg),
                    mqtt_conf->will_msg, mqtt_conf->qos, mqtt_conf->retain_birth_will == 1);

        do
        {
            ret = mosquitto_connect(mosq, mqtt_conf->host, mqtt_conf->port,
                                       mqtt_conf->keepalive);

            if(ret != MOSQ_ERR_SUCCESS)
                fprintf(stderr, "Unable to connect (%s).\n", mosquitto_strerror(ret));

            usleep(500*1000);

        } while(ret != MOSQ_ERR_SUCCESS);

        /* I'm not sure mosquitto_connect has already called the callback
           so I only change the state if it's not already connected
        */
        if (conn_state == CONN_DISCONNECTED)
            conn_state = CONN_CONNECTING;

        /*
         * Subscribe a topic.
         * mosq   - mosquitto instance
         * NULL   - optional message id
         * topic  - topic to subscribe
         * 0      - QoS Quality of service
         */
        if (debug) fprintf(stderr, "Subscribing topic %s\n", mqtt_conf->mqtt_prefix_cmnd);
        ret = mosquitto_subscribe(mosq, NULL, mqtt_conf->mqtt_prefix_cmnd, 0);
        if (ret != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "Unable to subscribe to the broker\n");
            break;
        }

        do
        {
            ret = mosquitto_loop(mosq, -1, 1);
            if (debug) fprintf(stderr, ".");
            if(conn_state != CONN_CONNECTED)
                usleep(100*1000);
        } while(conn_state == CONN_CONNECTING);

    }

    if (debug) fprintf(stderr, "\nconnected!\n");

    return 0;
}

int mqtt_send_message(mqtt_msg_t *msg, int retain)
{
    int ret;

    if(conn_state != CONN_CONNECTED)
        return -1;

    if (strlen(msg->topic) == 0) {
        fprintf(stderr, "No message sent: topic is empty\n");
        return -1;
    } else {
        ret = mosquitto_publish(mosq, &mid_sent, msg->topic, msg->len, msg->msg,
                              mqtt_conf->qos, retain);
    }

    if(ret != MOSQ_ERR_SUCCESS)
    {
        switch(ret){
        case MOSQ_ERR_INVAL:
            fprintf(stderr, "Error: Invalid input. Does your topic contain '+' or '#'?\n");
            break;
        case MOSQ_ERR_NOMEM:
            fprintf(stderr, "Error: Out of memory when trying to publish message.\n");
            break;
        case MOSQ_ERR_NO_CONN:
            fprintf(stderr, "Error: Client not connected when trying to publish.\n");
            break;
        case MOSQ_ERR_PROTOCOL:
            fprintf(stderr, "Error: Protocol error when communicating with broker.\n");
            break;
        case MOSQ_ERR_PAYLOAD_SIZE:
            fprintf(stderr, "Error: Message payload is too large.\n");
            break;
        }
    }
    return ret;
}

//-----------------------------------------------------------------------------

static int init_mosquitto_instance()
{
    mosq = mosquitto_new(mqtt_conf->client_id, true, NULL);

    if(!mosq){
        switch(errno){
        case ENOMEM:
            fprintf(stderr, "Error: Out of memory.\n");
            break;
        case EINVAL:
            fprintf(stderr, "Error: Invalid id.\n");
            break;
        }
        mosquitto_lib_cleanup();
        return -1;
    }

    mosquitto_log_callback_set(mosq, log_callback);

    mosquitto_connect_callback_set(mosq, connect_callback);
    mosquitto_disconnect_callback_set(mosq, disconnect_callback);
    mosquitto_publish_callback_set(mosq, publish_callback);
    mosquitto_message_callback_set(mosq, message_callback);

    return 0;
}

//-----------------------------------------------------------------------------

/*
 * connection callback
 * mosq   - mosquitto instance
 * obj    - user data for mosquitto_new
 * result - return code:
 *  0 - success
 *  1 - connection denied (incorrect protocol version)
 *  2 - connection denied (wrong id)
 *  3 - connection denied (broker not available)
 *  4-255 - reserved
 */
static void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    if (debug) fprintf(stderr, "Callback, connected with return code rc=%d\n", result);

    if(result == MOSQ_ERR_SUCCESS)
    {
        conn_state = CONN_CONNECTED;
        send_birth_msg();
    }
    else
    {
        conn_state = CONN_DISCONNECTED;
        fprintf(stderr, "%s\n", mosquitto_connack_string(result));
    }
}

static void disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
    if (debug) fprintf(stderr, "Callback, disconnected\n");

    conn_state = CONN_DISCONNECTED;
}

static void publish_callback(struct mosquitto *mosq, void *obj, int mid)
{
    /*
    last_mid_sent = mid;
    if(disconnect_sent == false)
    {
        mosquitto_disconnect(mosq);
        disconnect_sent = true;
    }
    */
}

static void log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
    if (debug) fprintf(stderr, "%s\n", str);
}

/*
 * message callback when a message is received from broker
 * mosq     - mosquitto instance
 * obj      - user data for mosquitto_new
 * message  - message data
 *               The library will free this variable and the related memory
 *               when callback terminates.
 *               The client must copy all needed data.
 */
static void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    char topic_prefix[MAX_LINE_LENGTH], topic[MAX_LINE_LENGTH], file[MAX_KEY_LENGTH], param[MAX_KEY_LENGTH];
    char *slash;
    bool match = 0;
    int len;
    char cmd_line[1024];

    if (debug) fprintf(stderr, "Received message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);
    /*
     * Check if the argument matches the subscription
     */
    mosquitto_topic_matches_sub(mqtt_conf->mqtt_prefix_cmnd, message->topic, &match);
    if (!match) {
        return;
    }

    /*
     * Check if the topic is in the form topic_prefix/file/parameter
     */
    memset(topic_prefix, '\0', MAX_LINE_LENGTH);
    memset(topic, '\0', MAX_LINE_LENGTH);
    memset(file, '\0', MAX_KEY_LENGTH);
    memset(param, '\0', MAX_KEY_LENGTH);

    // Remove /#
    len = strlen(mqtt_conf->mqtt_prefix_cmnd) - 2;
    if (len >= MAX_LINE_LENGTH) {
        if (debug) fprintf(stderr, "Message topic exceeds buffer size\n");
        return;
    }
    strncpy(topic_prefix, mqtt_conf->mqtt_prefix_cmnd, len);
    if (strncasecmp(topic_prefix, message->topic, len) != 0) {
        return;
    }
    if (strlen(message->topic) < len + 2) {
        return;
    }
    if (strchr(message->topic, '/') == NULL) {
        return;
    }
    strcpy(topic, &(message->topic)[len + 1]);
    slash = strchr(topic, '/');
    if (slash == NULL) {
        strcpy(file, topic);
        if (strlen(file) == 0) {
            fprintf(stderr, "Wrong message subtopic\n");
            return;
        }
        if ((message->payload == NULL) || (strlen(message->payload) == 0)) {
            // Send response with a dump of the configuration
            sprintf(cmd_line, "%s %s", CONF2MQTT_SCRIPT, file);
            if (debug) fprintf(stderr, "Running system command \"%s\"\n", cmd_line);
            system(cmd_line);
        }
        return;
    }
    if (slash - topic >= MAX_KEY_LENGTH) {
        fprintf(stderr, "Message subtopic exceeds buffer size\n");
        return;
    }
    strncpy(file, topic, slash - topic);
    if (strlen(slash + 1) >= MAX_KEY_LENGTH) {
        fprintf(stderr, "Message subtopic exceeds buffer size\n");
        return;
    }
    strcpy(param, slash + 1);
    if (strlen(param) == 0) {
        fprintf(stderr, "Param is empty\n");
        return;
    }
    if ((message->payload == NULL) || (strlen(message->payload) == 0)) {
        fprintf(stderr, "Payload is empty\n");
        return;
    }

    if (debug) fprintf(stderr, "Updating db: parameter \"%s\", value \"%s\"\n", param, (char *) message->payload);
    if (strcasecmp("switch_on", param) == 0) {
        if ((strcasecmp("on", (char *) message->payload) == 0) || (strcasecmp("yes", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -t on", IPC_CMD);
            system(cmd_line);
        } else if ((strcasecmp("off", (char *) message->payload) == 0) || (strcasecmp("no", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -t off", IPC_CMD);
            system(cmd_line);
        }
    } else if (strcasecmp("pan_left", param) == 0) {
        if ((strcasecmp("on", (char *) message->payload) == 0) || (strcasecmp("yes", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -a stop; %s -a left", PTZ_CMD, PTZ_CMD);
            system(cmd_line);
        } else if ((strcasecmp("off", (char *) message->payload) == 0) || (strcasecmp("no", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -a stop", PTZ_CMD);
            system(cmd_line);
        }
    } else if (strcasecmp("pan_right", param) == 0) {
        if ((strcasecmp("on", (char *) message->payload) == 0) || (strcasecmp("yes", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -a stop; %s -a right", PTZ_CMD, PTZ_CMD);
            system(cmd_line);
        } else if ((strcasecmp("off", (char *) message->payload) == 0) || (strcasecmp("no", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -a stop", PTZ_CMD);
            system(cmd_line);
        }
    } else if (strcasecmp("pan_up", param) == 0) {
        if ((strcasecmp("on", (char *) message->payload) == 0) || (strcasecmp("yes", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -a stop; %s -a up", PTZ_CMD, PTZ_CMD);
            system(cmd_line);
        } else if ((strcasecmp("off", (char *) message->payload) == 0) || (strcasecmp("no", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -a stop", PTZ_CMD);
            system(cmd_line);
        }
    } else if (strcasecmp("pan_down", param) == 0) {
        if ((strcasecmp("on", (char *) message->payload) == 0) || (strcasecmp("yes", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -a stop; %s -a down", PTZ_CMD, PTZ_CMD);
            system(cmd_line);
        } else if ((strcasecmp("off", (char *) message->payload) == 0) || (strcasecmp("no", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -a stop", PTZ_CMD);
            system(cmd_line);
        }
    } else if (strcasecmp("motion_detection", param) == 0) {
        if ((strcasecmp("on", (char *) message->payload) == 0) || (strcasecmp("yes", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -m on", IPC_CMD);
            system(cmd_line);
        } else if ((strcasecmp("off", (char *) message->payload) == 0) || (strcasecmp("no", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -m off", IPC_CMD);
            system(cmd_line);
        }
    } else if (strcasecmp("sensitivity", param) == 0) {
        if ((strcasecmp("low", (char *) message->payload) == 0) || (strcasecmp("25", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -s low", IPC_CMD);
            system(cmd_line);
        } else if ((strcasecmp("medium", (char *) message->payload) == 0) || (strcasecmp("50", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -s medium", IPC_CMD);
            system(cmd_line);
        } else if ((strcasecmp("high", (char *) message->payload) == 0) || (strcasecmp("75", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -s high", IPC_CMD);
            system(cmd_line);
        }
    } else if (strcasecmp("local_record", param) == 0) {
        if ((strcasecmp("on", (char *) message->payload) == 0) || (strcasecmp("yes", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -l on", IPC_CMD);
            system(cmd_line);
        } else if ((strcasecmp("off", (char *) message->payload) == 0) || (strcasecmp("no", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -l off", IPC_CMD);
            system(cmd_line);
        }
    } else if (strcasecmp("ir", param) == 0) {
        if (strcasecmp("auto", (char *) message->payload) == 0) {
            sprintf(cmd_line, "%s -i auto", IPC_CMD);
            system(cmd_line);
        } else if ((strcasecmp("on", (char *) message->payload) == 0) || (strcasecmp("yes", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -i on", IPC_CMD);
            system(cmd_line);
        } else if ((strcasecmp("off", (char *) message->payload) == 0) || (strcasecmp("no", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -i off", IPC_CMD);
            system(cmd_line);
        }
    } else if (strcasecmp("rotate", param) == 0) {
        if ((strcasecmp("on", (char *) message->payload) == 0) || (strcasecmp("yes", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -r on", IPC_CMD);
            system(cmd_line);
        } else if ((strcasecmp("off", (char *) message->payload) == 0) || (strcasecmp("no", (char *) message->payload) == 0)) {
            sprintf(cmd_line, "%s -r off", IPC_CMD);
            system(cmd_line);
        }
    }
}
#endif //USE_MOSQUITTO
