#include "mqtt-sonoff.h"

/*
 * Just a quick disclaimer.
 * This code is ugly and it is the result of some testing
 * and madness with the MQTT.
 *
 * It will be probably re-written from the ground-up to
 * handle more configurations, messages and callbacks.
 *
 * Warning: No clean exit.
 *
 * Crypto
 */

mqtt_conf_t conf;
mqtt_sonoff_conf_t mqtt_sonoff_conf;

static void init_mqtt_sonoff_config();
static void handle_config(const char *key, const char *value);

int files_delay = 70;        // Wait for xx seconds before search for mp4 files
int files_max_events = 50;   // Number of files reported in the message

void callback_motion_start()
{
    char topic[128];
    char cmd[128];
    char bufferFile[L_tmpnam];
    FILE *fImage;
    long int sz;
    char *bufferImage;
    mqtt_msg_t msg;

    printf("CALLBACK MOTION START\n");

    // Send start message
    msg.msg=mqtt_sonoff_conf.motion_start_msg;
    msg.len=strlen(msg.msg);
    msg.topic=topic;

    sprintf(topic, "%s/%s", mqtt_sonoff_conf.mqtt_prefix, mqtt_sonoff_conf.topic_motion);

    mqtt_send_message(&msg, conf.retain_motion);

    if (strcmp(EMPTY_TOPIC, mqtt_sonoff_conf.topic_motion_image) != 0) {
        // Send image
        tmpnam(bufferFile);
        sprintf(cmd, "%s -f %s", MQTT_SONOFF_SNAPSHOT, bufferFile);
        system(cmd);

        fImage = fopen(bufferFile, "r");
        if (fImage == NULL) {
            printf("Cannot open image file\n");
            remove(bufferFile);
            return;
        }
        fseek(fImage, 0L, SEEK_END);
        sz = ftell(fImage);
        fseek(fImage, 0L, SEEK_SET);

        bufferImage = (char *) malloc(sz * sizeof(char));
        if (bufferImage == NULL) {
            printf("Cannot allocate memory\n");
            fclose(fImage);
            remove(bufferFile);
            return;
        }
        if (fread(bufferImage, 1, sz, fImage) != sz) {
            printf("Cannot read image file\n");
            free(bufferImage);
            fclose(fImage);
            remove(bufferFile);
            return;
        }

        msg.msg=bufferImage;
        msg.len=sz;
        msg.topic=topic;

        sprintf(topic, "%s/%s", mqtt_sonoff_conf.mqtt_prefix, mqtt_sonoff_conf.topic_motion_image);

        mqtt_send_message(&msg, conf.retain_motion_image);

        // Clean
        free(bufferImage);
        fclose(fImage);
        remove(bufferFile);
    }

    printf("WAIT 10 S AND SEND MOTION STOP\n");
    sleep(10);

    // Send start message
    msg.msg=mqtt_sonoff_conf.motion_stop_msg;
    msg.len=strlen(msg.msg);
    msg.topic=topic;

    sprintf(topic, "%s/%s", mqtt_sonoff_conf.mqtt_prefix, mqtt_sonoff_conf.topic_motion);

    mqtt_send_message(&msg, conf.retain_motion);
}

int main(int argc, char **argv)
{
    int ret;

    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    printf("Starting mqtt_sonoff v%s\n", MQTT_SONOFF_VERSION);

    mqtt_init_conf(&conf);
    init_mqtt_sonoff_config();
    mqtt_set_conf(&conf);

    ret=init_mqtt();
    if(ret!=0)
        exit(EXIT_FAILURE);

    ret=mqtt_connect();
    if(ret!=0)
        exit(EXIT_FAILURE);

    ret=sql_init();
    if(ret!=0)
        exit(EXIT_FAILURE);

    sql_set_callback(SQL_MSG_MOTION_START, &callback_motion_start);

    while(1)
    {
        mqtt_check_connection();
        mqtt_loop();
        usleep(500*1000);
    }

    sql_stop();
    stop_mqtt();
    stop_config();

    return 0;
}

static void handle_config(const char *key, const char *value)
{
    int nvalue;

    // Ok, the configuration handling is UGLY, unsafe and repetitive.
    // It should be fixed in the future by writing a better config
    // handler or by just using a library.

    // If you think to have a better implementation.. PRs are welcome!

    if(strcmp(key, "MQTT_IP")==0)
    {
        strcpy(conf.host, value);
    }
    else if(strcmp(key, "MQTT_CLIENT_ID")==0)
    {
        strcpy(conf.client_id, value);
    }
    else if(strcmp(key, "MQTT_USER")==0)
    {
        conf.user=malloc((char)strlen(value)+1);
        strcpy(conf.user, value);
    }
    else if(strcmp(key, "MQTT_PASSWORD")==0)
    {
        conf.password=malloc((char)strlen(value)+1);
        strcpy(conf.password, value);
    }
    else if(strcmp(key, "MQTT_PORT")==0)
    {
        errno=0;
        nvalue=strtol(value, NULL, 10);
        if(errno==0)
            conf.port=nvalue;
    }
    else if(strcmp(key, "MQTT_KEEPALIVE")==0)
    {
        errno=0;
        nvalue=strtol(value, NULL, 10);
        if(errno==0)
            conf.keepalive=nvalue;
    }
    else if(strcmp(key, "MQTT_QOS")==0)
    {
        errno=0;
        nvalue=strtol(value, NULL, 10);
        if(errno==0)
            conf.qos=nvalue;
    }
    else if(strcmp(key, "MQTT_RETAIN_BIRTH_WILL")==0)
    {
        errno=0;
        nvalue=strtol(value, NULL, 10);
        if(errno==0)
            conf.retain_birth_will=nvalue;
    }
    else if(strcmp(key, "MQTT_RETAIN_MOTION")==0)
    {
        errno=0;
        nvalue=strtol(value, NULL, 10);
        if(errno==0)
            conf.retain_motion=nvalue;
    }
    else if(strcmp(key, "MQTT_RETAIN_MOTION_IMAGE")==0)
    {
        errno=0;
        nvalue=strtol(value, NULL, 10);
        if(errno==0)
            conf.retain_motion_image=nvalue;
    }
    else if(strcmp(key, "MQTT_PREFIX")==0)
    {
        conf.mqtt_prefix=malloc((char)strlen(value)+1);
        strcpy(conf.mqtt_prefix, value);
        mqtt_sonoff_conf.mqtt_prefix=malloc((char)strlen(value)+1);
        strcpy(mqtt_sonoff_conf.mqtt_prefix, value);
    }
    else if(strcmp(key, "TOPIC_BIRTH_WILL")==0)
    {
        conf.topic_birth_will=malloc((char)strlen(value)+1);
        strcpy(conf.topic_birth_will, value);
        mqtt_sonoff_conf.topic_birth_will=malloc((char)strlen(value)+1);
        strcpy(mqtt_sonoff_conf.topic_birth_will, value);
    }
    else if(strcmp(key, "TOPIC_MOTION")==0)
    {
        mqtt_sonoff_conf.topic_motion=malloc((char)strlen(value)+1);
        strcpy(mqtt_sonoff_conf.topic_motion, value);
    }
    else if(strcmp(key, "TOPIC_MOTION_IMAGE")==0)
    {
        mqtt_sonoff_conf.topic_motion_image=malloc((char)strlen(value)+1);
        strcpy(mqtt_sonoff_conf.topic_motion_image, value);
    }
    else if(strcmp(key, "BIRTH_MSG")==0)
    {
        conf.birth_msg=malloc((char)strlen(value)+1);
        strcpy(conf.birth_msg, value);
        mqtt_sonoff_conf.birth_msg=malloc((char)strlen(value)+1);
        strcpy(mqtt_sonoff_conf.birth_msg, value);
    }
    else if(strcmp(key, "WILL_MSG")==0)
    {
        conf.will_msg=malloc((char)strlen(value)+1);
        strcpy(conf.will_msg, value);
        mqtt_sonoff_conf.will_msg=malloc((char)strlen(value)+1);
        strcpy(mqtt_sonoff_conf.will_msg, value);
    }
    else if(strcmp(key, "MOTION_START_MSG")==0)
    {
        mqtt_sonoff_conf.motion_start_msg=malloc((char)strlen(value)+1);
        strcpy(mqtt_sonoff_conf.motion_start_msg, value);
    }
    else if(strcmp(key, "MOTION_STOP_MSG")==0)
    {
        mqtt_sonoff_conf.motion_stop_msg=malloc((char)strlen(value)+1);
        strcpy(mqtt_sonoff_conf.motion_stop_msg, value);
    }
    else
    {
        printf("key: %s | value: %s\n", key, value);
        fprintf(stderr, "Unrecognized config.\n");
    }
}

static void init_mqtt_sonoff_config()
{
    // Setting conf vars to NULL
    mqtt_sonoff_conf.mqtt_prefix=NULL;
    mqtt_sonoff_conf.topic_birth_will=NULL;
    mqtt_sonoff_conf.topic_motion=NULL;
    mqtt_sonoff_conf.topic_motion_image=NULL;
    mqtt_sonoff_conf.birth_msg=NULL;
    mqtt_sonoff_conf.will_msg=NULL;
    mqtt_sonoff_conf.motion_start_msg=NULL;
    mqtt_sonoff_conf.motion_stop_msg=NULL;

    if(init_config(MQTT_SONOFF_CONF_FILE)!=0)
    {
        printf("Cannot open config file. Skipping.\n");
        return;
    }

    config_set_handler(&handle_config);
    config_parse();

    // Setting default for all char* vars
    if(conf.mqtt_prefix == NULL)
    {
        conf.mqtt_prefix=malloc((char)strlen("sonoffcam")+1);
        strcpy(conf.mqtt_prefix, "sonoffcam");
    }
    if(mqtt_sonoff_conf.mqtt_prefix == NULL)
    {
        mqtt_sonoff_conf.mqtt_prefix=malloc((char)strlen("sonoffcam")+1);
        strcpy(mqtt_sonoff_conf.mqtt_prefix, "sonoffcam");
    }
    if(conf.topic_birth_will == NULL)
    {
        conf.topic_birth_will=malloc((char)strlen(EMPTY_TOPIC)+1);
        strcpy(conf.topic_birth_will, EMPTY_TOPIC);
    }
    if(mqtt_sonoff_conf.topic_birth_will == NULL)
    {
        mqtt_sonoff_conf.topic_birth_will=malloc((char)strlen(EMPTY_TOPIC)+1);
        strcpy(mqtt_sonoff_conf.topic_birth_will, EMPTY_TOPIC);
    }
    if(mqtt_sonoff_conf.topic_motion == NULL)
    {
        mqtt_sonoff_conf.topic_motion=malloc((char)strlen(EMPTY_TOPIC)+1);
        strcpy(mqtt_sonoff_conf.topic_motion, EMPTY_TOPIC);
    }
    if(mqtt_sonoff_conf.topic_motion_image == NULL)
    {
        mqtt_sonoff_conf.topic_motion_image=malloc((char)strlen(EMPTY_TOPIC)+1);
        strcpy(mqtt_sonoff_conf.topic_motion_image, EMPTY_TOPIC);
    }
    if(conf.birth_msg == NULL)
    {
        conf.birth_msg=malloc((char)strlen("online")+1);
        strcpy(conf.birth_msg, "online");
    }
    if(mqtt_sonoff_conf.birth_msg == NULL)
    {
        mqtt_sonoff_conf.birth_msg=malloc((char)strlen("online")+1);
        strcpy(mqtt_sonoff_conf.birth_msg, "online");
    }
    if(conf.will_msg == NULL)
    {
        conf.will_msg=malloc((char)strlen("offline")+1);
        strcpy(conf.will_msg, "offline");
    }
    if(mqtt_sonoff_conf.will_msg == NULL)
    {
        mqtt_sonoff_conf.will_msg=malloc((char)strlen("offline")+1);
        strcpy(mqtt_sonoff_conf.will_msg, "offline");
    }
    if(mqtt_sonoff_conf.motion_start_msg == NULL)
    {
        mqtt_sonoff_conf.motion_start_msg=malloc((char)strlen("motion_start")+1);
        strcpy(mqtt_sonoff_conf.motion_start_msg, "motion_start");
    }
    if(mqtt_sonoff_conf.motion_stop_msg == NULL)
    {
        mqtt_sonoff_conf.motion_stop_msg=malloc((char)strlen("motion_stop")+1);
        strcpy(mqtt_sonoff_conf.motion_stop_msg, "motion_stop");
    }
}
