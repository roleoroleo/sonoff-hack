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
static void init_sonoff_colink_config();
static void handle_colink_config(const char *key, const char *value);

static void send_ha_discovery();
static char *print_motion_json();
static char *print_image_json();
static int json_common(cJSON *confObject, const char **suffix);

char *default_prefix = "sonoffcam";
char *default_online = "online";
char *default_offline = "offline";
char *default_start = "motion_start";
char *default_stop = "motion_stop";
char *default_topic = EMPTY_TOPIC;

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
    mqtt_set_conf(&conf);
    init_mqtt_sonoff_config();
    init_sonoff_colink_config();

    ret=init_mqtt();
    if(ret!=0)
        exit(EXIT_FAILURE);

    ret=mqtt_connect();
    if(ret!=0)
        exit(EXIT_FAILURE);

    send_ha_discovery();

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
    else if(strcmp(key, "HA_DISCOVERY")==0)
    {
        conf_set_int(value, &mqtt_sonoff_conf.ha_enable_discovery);
    }
    else if(strcmp(key, "HA_NAME_PREFIX")==0)
    {
        mqtt_sonoff_conf.ha_name_prefix=conf_set_string(value);
    }
    else if(strcmp(key, "HA_CONF_PREFIX")==0)
    {
        mqtt_sonoff_conf.ha_conf_prefix=conf_set_string(value);
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
    mqtt_sonoff_conf.ha_conf_prefix=NULL;
    mqtt_sonoff_conf.ha_name_prefix=NULL;

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
        conf.mqtt_prefix=default_prefix;
        mqtt_sonoff_conf.mqtt_prefix=conf.mqtt_prefix;
    }

    if(conf.topic_birth_will == NULL)
    {
        conf.topic_birth_will=default_topic;
        mqtt_sonoff_conf.topic_birth_will=conf.topic_birth_will;
    }
    if(mqtt_sonoff_conf.topic_motion == NULL)
    {
        mqtt_sonoff_conf.topic_motion=default_topic;
    }
    if(mqtt_sonoff_conf.topic_motion_image == NULL)
    {
        mqtt_sonoff_conf.topic_motion_image=default_topic;
    }
    if(conf.birth_msg == NULL)
    {
        conf.birth_msg=default_online;
        mqtt_sonoff_conf.birth_msg=conf.birth_msg;
    }
    if(conf.will_msg == NULL)
    {
        conf.will_msg=default_offline;
        mqtt_sonoff_conf.will_msg=conf.will_msg;
    }
    if(mqtt_sonoff_conf.motion_start_msg == NULL)
    {
        mqtt_sonoff_conf.motion_start_msg=default_start;
    }
    if(mqtt_sonoff_conf.motion_stop_msg == NULL)
    {
        mqtt_sonoff_conf.motion_stop_msg=default_stop;
    }
}

static void init_sonoff_colink_config(){
    mqtt_sonoff_conf.device_id=NULL;
    mqtt_sonoff_conf.device_model=NULL;

    if(init_config(COLINK_CONF_FILE)!=0)
    {
        printf("Cannot open config file. Skipping.\n");
        return;
    }

    config_set_handler(&handle_colink_config);
    config_parse();
    stop_config();
}

static void handle_colink_config(const char *key, const char *value)
{

    char *tmpValue=conf_set_string(value);

    //strip quote from end of value
    tmpValue[strlen(tmpValue)-1] = '\0';

    if(strcmp(key, "devid")==0)
    {
        mqtt_sonoff_conf.device_id=conf_set_string(tmpValue+1); //skip leading "
    }
    else if(strcmp(key, "model")==0)
    {
        mqtt_sonoff_conf.device_model=conf_set_string(tmpValue+1);
    }

    free(tmpValue);
}
static void send_ha_discovery() {
    char topic[128];
    const int retain = true;
    mqtt_msg_t msg;
    msg.topic=topic;

    if (mqtt_sonoff_conf.ha_enable_discovery) {
        // Send motion config
        msg.msg=print_motion_json();
        cJSON_Minify(msg.msg);
        msg.len=strlen(msg.msg);

        sprintf(topic, "%s/binary_sensor/%s_motion/config", mqtt_sonoff_conf.ha_conf_prefix, mqtt_sonoff_conf.device_id);
        mqtt_send_message(&msg, retain);
        free(msg.msg);

        // Send motion image config
        msg.msg=print_image_json();
        cJSON_Minify(msg.msg);
        msg.len=strlen(msg.msg);

        sprintf(topic, "%s/camera/%s_image/config", mqtt_sonoff_conf.ha_conf_prefix, mqtt_sonoff_conf.device_id);
        mqtt_send_message(&msg, retain);
        free(msg.msg);
    }
}

static char *print_motion_json() {
    const char *suffix = "motion";
    char *json = NULL;
    cJSON * confObject = cJSON_CreateObject();

    char stopic[128];
    sprintf(stopic, "%s/%s", mqtt_sonoff_conf.mqtt_prefix, mqtt_sonoff_conf.topic_motion);
    if (cJSON_AddStringToObject(confObject, "state_topic", stopic) == NULL) {
        goto end;
    }
    if (json_common(confObject, &suffix) == 0) {
        goto end;
    }
    if (cJSON_AddStringToObject(confObject, "device_class", "motion") == NULL) {
        goto end;
    }
    if (cJSON_AddStringToObject(confObject, "payload_on", mqtt_sonoff_conf.motion_start_msg) == NULL) {
        goto end;
    }
    if (cJSON_AddStringToObject(confObject, "payload_off", mqtt_sonoff_conf.motion_stop_msg) == NULL) {
        goto end;
    }

    json = cJSON_Print(confObject);
    if (json == NULL) {
        fprintf(stderr, "Error preparing HA discovery config");
    }

end:
    cJSON_Delete(confObject);
    return json;
}

static char *print_image_json() {

    const char *suffix = "image";
    char *json = NULL;
    cJSON * confObject = cJSON_CreateObject();

    char stopic[128];
    sprintf(stopic, "%s/%s", mqtt_sonoff_conf.mqtt_prefix, mqtt_sonoff_conf.topic_motion_image);
    if (cJSON_AddStringToObject(confObject, "topic", stopic) == NULL) {
        goto end;
    }
    if (json_common(confObject, &suffix) == 0) {
        goto end;
    }
    if (cJSON_AddStringToObject(confObject, "icon", "mdi:camera") == NULL) {
        goto end;
    }

    json = cJSON_Print(confObject);
    if (json == NULL) {
        fprintf(stderr, "Error preparing HA discovery config");
    }


end:
    cJSON_Delete(confObject);
    return json;
}

static int json_common(cJSON *confObject, const char **suffix) {

    char dname[128];
    if (mqtt_sonoff_conf.ha_name_prefix) {
        sprintf(dname, "%s %s", mqtt_sonoff_conf.ha_name_prefix, *suffix);
    } else {
        strcpy(dname, *suffix);
    }
    if (cJSON_AddStringToObject(confObject, "name", dname) == NULL) {
        goto end;
    }
    cJSON *availArray = cJSON_AddArrayToObject(confObject, "availability");
    if (availArray == NULL ) {
        goto end;
    }
    cJSON *availObj = cJSON_CreateObject();
    if (availObj == NULL ) {
        goto end;
    }
    cJSON_AddItemToArray(availArray, availObj);

    char bwtopic[128];
    sprintf(bwtopic, "%s/%s", mqtt_sonoff_conf.mqtt_prefix, mqtt_sonoff_conf.topic_birth_will);
    if (cJSON_AddStringToObject(availObj, "topic", bwtopic) == NULL) {
        goto end;
    }
    if (strcmp(mqtt_sonoff_conf.birth_msg, "online") != 0) {
        if (cJSON_AddStringToObject(availObj, "payload_available", mqtt_sonoff_conf.birth_msg) == NULL) {
            goto end;
        }
    }
    if (strcmp(mqtt_sonoff_conf.will_msg, "offline") != 0) {
        if (cJSON_AddStringToObject(availObj, "payload_not_available", mqtt_sonoff_conf.will_msg) == NULL) {
            goto end;
        }
    }
    //build device object for device registry association
    cJSON *deviceObj = cJSON_AddObjectToObject(confObject, "device");
    if (deviceObj == NULL ) {
        goto end;
    }
    cJSON *idents = cJSON_AddArrayToObject(deviceObj, "identifiers");
    if (idents == NULL ) {
        goto end;
    }
    cJSON *ident_string = cJSON_CreateString(mqtt_sonoff_conf.device_id);
    if (ident_string == NULL ) {
        goto end;
    }
    cJSON_AddItemToArray(idents, ident_string);

    if (mqtt_sonoff_conf.ha_name_prefix){
        if (cJSON_AddStringToObject(deviceObj, "name", mqtt_sonoff_conf.ha_name_prefix) == NULL) {
            goto end;
        }
    }

    if (cJSON_AddStringToObject(deviceObj, "manufacturer", "Sonoff") == NULL) {
        goto end;
    }
    if (cJSON_AddStringToObject(deviceObj, "model", mqtt_sonoff_conf.device_model) == NULL) {
        goto end;
    }

    char uuid[128];
    sprintf(uuid, "%s_%s", mqtt_sonoff_conf.device_id, *suffix);
    if (cJSON_AddStringToObject(confObject, "unique_id", uuid) == NULL) {
        goto end;
    }
    if (cJSON_AddNumberToObject(confObject, "qos", conf.qos) == NULL) {
        goto end;
    }
    return 1;
end:
    printf("Error generating common json config payload");
    return 0;

}