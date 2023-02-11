#include "mqtt-config.h"

struct mosquitto *mosq;
enum conn_states {CONN_DISCONNECTED, CONN_CONNECTING, CONN_CONNECTED};
static enum conn_states conn_state;

mqtt_conf_t conf;

int run;
int debug = 1;

/*
 * signal handler to terminate program
 */
void handle_signal(int s)
{
    run = 0;
    printf("Signal Handler\n");
}

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
void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    printf("Callback, connected with return code rc=%d\n", result);

    if(result==MOSQ_ERR_SUCCESS)
    {
        conn_state=CONN_CONNECTED;
    }
    else
    {
        conn_state=CONN_DISCONNECTED;
    }
}

void disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
    conn_state=CONN_DISCONNECTED;
}

/*
 * message callback when a message is published
 * mosq     - mosquitto instance
 * obj      - user data for mosquitto_new
 * message  - message data
 *               The library will free this variable and the related memory
 *               when callback terminates.
 *               The client must copy all needed data.
 */
void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    char topic_prefix[MAX_LINE_LENGTH], topic[MAX_LINE_LENGTH], file[MAX_KEY_LENGTH], param[MAX_KEY_LENGTH];
    char *slash;
    bool match = 0;
    int len;
    char sys_cmd[128];

    if (debug) printf("Received message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);
    /*
     * Check if the argument matches the subscription
     */
    mosquitto_topic_matches_sub(conf.mqtt_prefix_cmnd, message->topic, &match);
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
    len = strlen(conf.mqtt_prefix_cmnd) - 2;
    if (len >= MAX_LINE_LENGTH) {
        if (debug) printf("Message topic exceeds buffer size\n");
        return;
    }
    strncpy(topic_prefix, conf.mqtt_prefix_cmnd, len);
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
        return;
    }
    if (slash - topic >= MAX_KEY_LENGTH) {
        if (debug) printf("Message subtopic exceeds buffer size\n");
        return;
    }
    strncpy(file, topic, slash - topic);
    if (strlen(slash + 1) >= MAX_KEY_LENGTH) {
        if (debug) printf("Message subtopic exceeds buffer size\n");
        return;
    }
    strcpy(param, slash + 1);
    if (strlen(param) == 0) {
        if (debug) printf("Param is empty\n");
        return;
    }
    if ((message->payload == NULL) || (strlen(message->payload) == 0)) {
        if (debug) printf("Payload is empty\n");
        return;
    }

    if (debug) printf("Updating db: parameter \"%s\", value \"%s\"\n", param, (char *) message->payload);
    if (sql_update(param, (char *) message->payload) != 0) {
        printf("Update error: parameter \"%s\", value \"%s\"\n", param, (char *) message->payload);
    }

    if (strcasecmp("switch_on", param) == 0) {
        if ((strcasecmp("on", (char *) message->payload) == 0) || (strcasecmp("yes", (char *) message->payload) == 0)) {
            sprintf(sys_cmd, "%s off", SWITCH_ON_SCRIPT);
            system(sys_cmd);
        } else if ((strcasecmp("off", (char *) message->payload) == 0) || (strcasecmp("no", (char *) message->payload) == 0)) {
            sprintf(sys_cmd, "%s on", SWITCH_ON_SCRIPT);
            system(sys_cmd);
        }
    }
}

void handle_config(const char *key, const char *value)
{
    int nvalue;

    if(strcmp(key, "MQTT_IP") == 0) {
        if (strlen(value) < sizeof(conf.host))
            strcpy(conf.host, value);
    } else if(strcmp(key, "MQTT_CLIENT_ID")==0) {
        conf.client_id = malloc((char) strlen(value) + 1 + 2);
        sprintf(conf.client_id, "%s_c", value);
    } else if(strcmp(key, "MQTT_PREFIX")==0) {
        conf.mqtt_prefix_cmnd = malloc((char) strlen(value) + 1 + 7);
        sprintf(conf.mqtt_prefix_cmnd, "%s/cmnd/#", value);
    } else if(strcmp(key, "MQTT_USER") == 0) {
        conf.user = malloc((char) strlen(value) + 1);
        strcpy(conf.user, value);
    } else if(strcmp(key, "MQTT_PASSWORD") == 0) {
        conf.password = malloc((char) strlen(value) + 1);
        strcpy(conf.password, value);
    } else if(strcmp(key, "MQTT_PORT") == 0) {
        errno = 0;
        nvalue = strtol(value, NULL, 10);
        if(errno == 0)
            conf.port = nvalue;
    } else {
        if (debug) {
            printf("Ignoring key: %s - value: %s\n", key, value);
        }
    }
}

int mqtt_init_conf(mqtt_conf_t *conf)
{
    FILE *fp;

    strcpy(conf->host, "127.0.0.1");
    strcpy(conf->bind_address, "0.0.0.0");
    conf->user = NULL;
    conf->password = NULL;
    conf->port = 1883;
    conf->mqtt_prefix_cmnd = NULL;

    fp = open_conf_file(MQTT_CONF_FILE);
    if (fp == NULL)
        return -1;

    config_set_handler(&handle_config);
    config_parse(fp);

    close_conf_file(fp);

    return 0;
}

void mqtt_check_connection()
{
    if (conn_state != CONN_CONNECTED) {
        mqtt_connect();
    }
}

int mqtt_connect()
{
    int rc = 0;

    /*
     * Set connection credentials.
     */
    if (conf.user!=NULL && strcmp(conf.user, "") != 0) {
        rc = mosquitto_username_pw_set(mosq, conf.user, conf.password);
        if(rc != MOSQ_ERR_SUCCESS) {
            printf("Unable to set the auth parameters (%s).\n", mosquitto_strerror(rc));
            return -2;
        }
    }

    /*
     * Set connection callback.
     * It's called when the broker send CONNACK message.
     */
    mosquitto_connect_callback_set(mosq, connect_callback);
    /*
     * Set disconnection callback.
     * It's called at the disconnection.
     */
    mosquitto_disconnect_callback_set(mosq, disconnect_callback);
    /*
     * Set subscription callback.
     * It's called when the broker send a subscribed message.
     */
    mosquitto_message_callback_set(mosq, message_callback);
    /*
     * Connect to the broker.
     */
    conn_state = CONN_DISCONNECTED;

    while (conn_state != CONN_CONNECTED) {

        if (debug) printf("Connecting to broker %s\n", conf.host);
        do
        {
            rc = mosquitto_connect(mosq, conf.host, conf.port, 15);

            if(rc != MOSQ_ERR_SUCCESS)
                printf("Unable to connect (%s).\n", mosquitto_strerror(rc));

            usleep(500*1000);

        } while(rc != MOSQ_ERR_SUCCESS);

        if (conn_state == CONN_DISCONNECTED)
            conn_state = CONN_CONNECTING;

        /*
         * Subscribe a topic.
         * mosq   - mosquitto instance
         * NULL   - optional message id
         * topic  - topic to subscribe
         * 0      - QoS Quality of service
         */
        if (debug) printf("Subscribing topic %s\n", conf.mqtt_prefix_cmnd);
        rc = mosquitto_subscribe(mosq, NULL, conf.mqtt_prefix_cmnd, 0);
        if (rc != MOSQ_ERR_SUCCESS) {
            printf("Unable to subscribe to the broker\n");
            break;
        }

        if (conn_state == CONN_CONNECTING)
            conn_state = CONN_CONNECTED;

        /*
         * Infinite loop waiting for incoming messages
         */
        do
        {
            rc = mosquitto_loop(mosq, -1, 1);
            if(conn_state != CONN_CONNECTED)
                usleep(100*1000);
        } while(conn_state == CONN_CONNECTING);
    }

    return 0;
}

void stop_mqtt(void)
{
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}

int main(int argc, char *argv[])
{
    run = 1;

    if (mqtt_init_conf(&conf) < 0)
        return -2;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    /*
     * Initialize library. Call it before any other function.
     */
    mosquitto_lib_init();
    sql_init();

    /*
     * Create a new client instance
     */
    mosq = mosquitto_new(conf.client_id, true, 0);

    if(mosq) {
        if (mqtt_connect() != 0)
            return -3;

        while(run)
        {
            mqtt_check_connection();
            mosquitto_loop(mosq, -1, 1);
            usleep(500*1000);
        }

        stop_mqtt();
    }

    sql_stop();

    return 0;
}