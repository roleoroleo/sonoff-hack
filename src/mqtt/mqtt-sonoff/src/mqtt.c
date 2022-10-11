#include "mqtt.h"

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

void send_birth_msg()
{
    char topic[128];
    mqtt_msg_t msg;

    msg.msg=mqtt_conf->birth_msg;
    msg.len=strlen(msg.msg);
    msg.topic=topic;

    sprintf(topic, "%s/%s", mqtt_conf->mqtt_prefix, mqtt_conf->topic_birth_will);

    mqtt_send_message(&msg, mqtt_conf->retain_birth_will);
}

void send_will_msg()
{
    char topic[128];
    mqtt_msg_t msg;

    msg.msg=mqtt_conf->will_msg;
    msg.len=strlen(msg.msg);
    msg.topic=topic;

    sprintf(topic, "%s/%s", mqtt_conf->mqtt_prefix, mqtt_conf->topic_birth_will);

    mqtt_send_message(&msg, mqtt_conf->retain_birth_will);
}

int init_mqtt(void)
{
    int ret;

    ret=mosquitto_lib_init();
    if(ret!=0)
    {
        fprintf(stderr, "Can't initialize mosquitto library.\n");
        return -1;
    }

    ret=init_mosquitto_instance();
    if(ret!=0)
    {
        fprintf(stderr, "Can't create mosquitto instance.\n");
        return -2;
    }

    conn_state=CONN_DISCONNECTED;

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
    if(mosq!=NULL)
        mosquitto_loop(mosq, -1, 1);
}

//-----------------------------------------------------------------------------

void mqtt_init_conf(mqtt_conf_t *conf)
{
    strcpy(conf->client_id, "sonoff-cam");

    conf->user=NULL;
    conf->password=NULL;

    strcpy(conf->host, "127.0.0.1");
    strcpy(conf->bind_address, "0.0.0.0");

    conf->port=1883;
    conf->keepalive=120;
    conf->qos=1;
    conf->retain_birth_will=1;
    conf->retain_motion=1;
    conf->retain_motion_image=1;
    conf->ipcsys_db=1;

    conf->mqtt_prefix=NULL;
    conf->topic_birth_will=NULL;
    conf->birth_msg=NULL;
    conf->will_msg=NULL;
}

void mqtt_set_conf(mqtt_conf_t *conf)
{
    mqtt_conf=conf;
}

void mqtt_check_connection()
{
    char topic[128];

    if (conn_state != CONN_CONNECTED) {
        sprintf(topic, "%s/%s", mqtt_conf->mqtt_prefix, mqtt_conf->topic_birth_will);
        mosquitto_will_set(mosq, topic, strlen(mqtt_conf->will_msg),
                    mqtt_conf->will_msg, mqtt_conf->qos, mqtt_conf->retain_birth_will == 1);
        mqtt_connect();
    }
}

int mqtt_connect()
{
    int ret;
    char topic[128];

    printf("Trying to connect... ");

    if(mqtt_conf->user!=NULL && strcmp(mqtt_conf->user, "")!=0)
    {
        ret=mosquitto_username_pw_set(mosq, mqtt_conf->user, mqtt_conf->password);

        if(ret!=MOSQ_ERR_SUCCESS)
        {
            fprintf(stderr, "Unable to set the auth parameters (%s).\n", mosquitto_strerror(ret));
            return -1;
        }
    }

    conn_state=CONN_DISCONNECTED;


    while (conn_state!=CONN_CONNECTED) {
        sprintf(topic, "%s/%s", mqtt_conf->mqtt_prefix, mqtt_conf->topic_birth_will);
        mosquitto_will_set(mosq, topic, strlen(mqtt_conf->will_msg),
                    mqtt_conf->will_msg, mqtt_conf->qos, mqtt_conf->retain_birth_will == 1);

        do
        {
            ret=mosquitto_connect(mosq, mqtt_conf->host, mqtt_conf->port,
                                       mqtt_conf->keepalive);

            if(ret!=MOSQ_ERR_SUCCESS)
                fprintf(stderr, "Unable to connect (%s).\n", mosquitto_strerror(ret));

            usleep(500*1000);

        } while(ret!=MOSQ_ERR_SUCCESS);

        /* I'm not sure mosquitto_connect has already called the callback
           so I only change the state if it's not already connected
        */
        if (conn_state==CONN_DISCONNECTED)
            conn_state=CONN_CONNECTING;

        do
        {
            ret=mosquitto_loop(mosq, -1, 1);
            printf(".");
            if(conn_state!=CONN_CONNECTED)
                usleep(100*1000);
        } while(conn_state==CONN_CONNECTING);

    }

    printf("\nconnected!\n");

    return 0;
}

int mqtt_send_message(mqtt_msg_t *msg, int retain)
{
    int ret;

    if(conn_state!=CONN_CONNECTED)
        return -1;

    if (strcmp("/", &msg->topic[strlen(msg->topic)-1])==0) {
        fprintf(stderr, "No message sent: topic is empty\n");
        return -1;
    } else {
        ret=mosquitto_publish(mosq, &mid_sent, msg->topic, msg->len, msg->msg,
                              mqtt_conf->qos, retain);
    }

    if(ret!=MOSQ_ERR_SUCCESS)
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

    return 0;
}

//-----------------------------------------------------------------------------

static void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    if(result==MOSQ_ERR_SUCCESS)
    {
        conn_state=CONN_CONNECTED;
        send_birth_msg();
    }
    else
    {
        conn_state=CONN_DISCONNECTED;
        fprintf(stderr, "%s\n", mosquitto_connack_string(result));
    }
}

static void disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
    conn_state=CONN_DISCONNECTED;
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
    printf("%s\n", str);
}
