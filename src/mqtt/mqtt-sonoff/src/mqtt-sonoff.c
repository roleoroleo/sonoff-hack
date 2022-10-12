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

    if (strlen(mqtt_sonoff_conf.topic_motion_image)) {
        // Send image
        printf("Wait %.1f seconds and take a snapshot\n", mqtt_sonoff_conf.motion_image_delay);
        tmpnam(bufferFile);
        sprintf(cmd, "%s -f %s", MQTT_SONOFF_SNAPSHOT, bufferFile);
        usleep((unsigned int) (mqtt_sonoff_conf.motion_image_delay * 1000.0 * 1000.0));
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

    ret=sql_init(conf.ipcsys_db);
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

    return 0;
}

static void handle_config(const char *key, const char *value)
{

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
        conf.user=conf_set_string(value);
    }
    else if(strcmp(key, "MQTT_PASSWORD")==0)
    {
        conf.password=conf_set_string(value);
    }
    else if(strcmp(key, "MQTT_PORT")==0)
    {
        conf_set_int(value, &conf.port);
    }
    else if(strcmp(key, "MQTT_KEEPALIVE")==0)
    {
        conf_set_int(value, &conf.keepalive);
    }
    else if(strcmp(key, "MQTT_QOS")==0)
    {
        conf_set_int(value, &conf.qos);
    }
    else if(strcmp(key, "MQTT_RETAIN_BIRTH_WILL")==0)
    {
        conf_set_int(value, &conf.retain_birth_will);
    }
    else if(strcmp(key, "MQTT_RETAIN_MOTION")==0)
    {
        conf_set_int(value, &conf.retain_motion);
    }
    else if(strcmp(key, "MQTT_RETAIN_MOTION_IMAGE")==0)
    {
        conf_set_int(value, &conf.retain_motion_image);
    }
    else if(strcmp(key, "MQTT_IPCSYS_DB")==0)
    {
        conf_set_int(value, &conf.ipcsys_db);
    }
    else if(strcmp(key, "MQTT_PREFIX")==0)
    {
        conf.mqtt_prefix=conf_set_string(value);
        mqtt_sonoff_conf.mqtt_prefix=conf.mqtt_prefix;
    }
    else if(strcmp(key, "TOPIC_BIRTH_WILL")==0)
    {
        conf.topic_birth_will=conf_set_string(value);
        mqtt_sonoff_conf.topic_birth_will=conf.topic_birth_will;
    }
    else if(strcmp(key, "TOPIC_MOTION")==0)
    {
        mqtt_sonoff_conf.topic_motion=conf_set_string(value);
    }
    else if(strcmp(key, "TOPIC_MOTION_IMAGE")==0)
    {
        mqtt_sonoff_conf.topic_motion_image=conf_set_string(value);
    }
    else if(strcmp(key, "MOTION_IMAGE_DELAY")==0)
    {
        conf_set_double(value, &mqtt_sonoff_conf.motion_image_delay);
    }
    else if(strcmp(key, "BIRTH_MSG")==0)
    {
        conf.birth_msg=conf_set_string(value);
        mqtt_sonoff_conf.birth_msg=conf.birth_msg;
    }
    else if(strcmp(key, "WILL_MSG")==0)
    {
        conf.will_msg=conf_set_string(value);
        mqtt_sonoff_conf.will_msg=conf.will_msg;
    }
    else if(strcmp(key, "MOTION_START_MSG")==0)
    {
        mqtt_sonoff_conf.motion_start_msg=conf_set_string(value);
    }
    else if(strcmp(key, "MOTION_STOP_MSG")==0)
    {
        mqtt_sonoff_conf.motion_stop_msg=conf_set_string(value);
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
    mqtt_sonoff_conf.motion_image_delay=0.5;
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
    stop_config();

    // Setting default for all char* vars
    if(conf.mqtt_prefix == NULL)
    {
        conf.mqtt_prefix=conf_set_string("sonoffcam");
        mqtt_sonoff_conf.mqtt_prefix=conf.mqtt_prefix;
    }

    if(conf.topic_birth_will == NULL)
    {
        conf.topic_birth_will=conf_set_string(EMPTY_TOPIC);
        mqtt_sonoff_conf.topic_birth_will=conf.topic_birth_will;
    }
    if(mqtt_sonoff_conf.topic_motion == NULL)
    {
        mqtt_sonoff_conf.topic_motion=conf_set_string(EMPTY_TOPIC);
    }
    if(mqtt_sonoff_conf.topic_motion_image == NULL)
    {
        mqtt_sonoff_conf.topic_motion_image=conf_set_string(EMPTY_TOPIC);
    }
    if(conf.birth_msg == NULL)
    {
        conf.birth_msg=conf_set_string("online");
        mqtt_sonoff_conf.birth_msg=conf.birth_msg;
    }
    if(conf.will_msg == NULL)
    {
        conf.will_msg=conf_set_string("offline");
        mqtt_sonoff_conf.will_msg=conf.will_msg;
    }
    if(mqtt_sonoff_conf.motion_start_msg == NULL)
    {
        mqtt_sonoff_conf.motion_start_msg=conf_set_string("motion_start");
    }
    if(mqtt_sonoff_conf.motion_stop_msg == NULL)
    {
        mqtt_sonoff_conf.motion_stop_msg=conf_set_string("motion_stop");
    }
}
