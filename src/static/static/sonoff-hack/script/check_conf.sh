#!/bin/sh

SYSTEM_CONF_FILE="/mnt/mmc/sonoff-hack/etc/system.conf"
CAMERA_CONF_FILE="/mnt/mmc/sonoff-hack/etc/camera.conf"
MQTTS_CONF_FILE="/mnt/mmc/sonoff-hack/etc/mqtt-sonoff.conf"

PARMS1="
HTTPD=yes
TELNETD=yes
SSHD=yes
FTPD=yes
BUSYBOX_FTPD=no
DISABLE_CLOUD=no
MQTT=no
ONVIF=yes
ONVIF_WSDD=yes
ONVIF_PROFILE=high
ONVIF_INTERFACE=eth0
NTPD=yes
NTP_SERVER=pool.ntp.org
RTSP_PORT=554
ONVIF_PORT=80
HTTPD_PORT=8080
USERNAME=
PASSWORD=
FREE_SPACE=0
FTP_UPLOAD=no
FTP_HOST=
FTP_USERNAME=
FTP_PASSWORD=
SSH_PASSWORD=
TIMEZONE=
SWAP_FILE=no"

PARMS2="
SENSITIVITY=low
ROTATE=no"

PARMS3="
MQTT_IP=0.0.0.0
MQTT_PORT=1883
MQTT_CLIENT_ID=sonoff-cam
MQTT_USER=
MQTT_PASSWORD=
MQTT_PREFIX=sonoffcam
TOPIC_MOTION=motion_detection
TOPIC_MOTION_FILES=motion_files
TOPIC_BABY_CRYING=baby_crying
MOTION_START_MSG=motion_start
MOTION_STOP_MSG=motion_stop
BABY_CRYING_MSG=crying
MQTT_KEEPALIVE=120
MQTT_QOS=1
MQTT_RETAIN_MOTION=1
MQTT_RETAIN_MOTION_FILES=1
MQTT_RETAIN_BABY_CRYING=1"

for i in $PARMS1
do
    if [ ! -z "$i" ]; then
        PAR=$(echo "$i" | cut -d= -f1)
        MATCH=$(cat $SYSTEM_CONF_FILE | grep $PAR)
        if [ -z "$MATCH" ]; then
            echo "$i" >> $SYSTEM_CONF_FILE
        fi
    fi
done

for i in $PARMS2
do
    if [ ! -z "$i" ]; then
        PAR=$(echo "$i" | cut -d= -f1)
        MATCH=$(cat $CAMERA_CONF_FILE | grep $PAR)
        if [ -z "$MATCH" ]; then
            echo "$i" >> $CAMERA_CONF_FILE
        fi
    fi
done

#for i in $PARMS3
#do
#    if [ ! -z "$i" ]; then
#        PAR=$(echo "$i" | cut -d= -f1)
#        MATCH=$(cat $MQTTV4_CONF_FILE | grep $PAR)
#        if [ -z "$MATCH" ]; then
#            echo "$i" >> $MQTTV4_CONF_FILE
#        fi
#    fi
#done
