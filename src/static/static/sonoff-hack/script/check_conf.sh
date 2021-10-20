#!/bin/sh

SYSTEM_CONF_FILE="/mnt/mmc/sonoff-hack/etc/system.conf"
CAMERA_CONF_FILE="/mnt/mmc/sonoff-hack/etc/camera.conf"
MQTT_SONOFF_CONF_FILE="/mnt/mmc/sonoff-hack/etc/mqtt-sonoff.conf"

PARMS1="
HTTPD=yes
TELNETD=yes
SSHD=yes
FTPD=yes
BUSYBOX_FTPD=no
DISABLE_CLOUD=no
MQTT=no
SNAPSHOT=yes
ONVIF=yes
ONVIF_WSDD=yes
ONVIF_PROFILE=high
ONVIF_NETIF=eth0
NTPD=yes
NTP_SERVER=pool.ntp.org
RTSP_PORT=554
ONVIF_PORT=1000
HTTPD_PORT=80
USERNAME=
PASSWORD=
FREE_SPACE=0
FTP_UPLOAD=no
FTP_HOST=
FTP_DIR=
FTP_DIR_TREE=no
FTP_USERNAME=
FTP_PASSWORD=
FTP_FILE_DELETE_AFTER_UPLOAD=yes
SSH_PASSWORD=
TIMEZONE=
SWAP_FILE=no
PTZ_PRESET_BOOT=default
CRONTAB="

PARMS2=""

PARMS3="
MQTT_IP=0.0.0.0
MQTT_PORT=1883
MQTT_CLIENT_ID=sonoff-cam
MQTT_USER=
MQTT_PASSWORD=
MQTT_PREFIX=sonoffcam
TOPIC_BIRTH_WILL=status
TOPIC_MOTION=motion_detection
TOPIC_MOTION_IMAGE=motion_detection_image
BIRTH_MSG=online
WILL_MSG=offline
MOTION_START_MSG=motion_start
MOTION_STOP_MSG=motion_stop
MQTT_KEEPALIVE=120
MQTT_QOS=1
MQTT_RETAIN_BIRTH_WILL=1
MQTT_RETAIN_MOTION=0
MQTT_RETAIN_MOTION_IMAGE=0"

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

for i in $PARMS3
do
    if [ ! -z "$i" ]; then
        PAR=$(echo "$i" | cut -d= -f1)
        MATCH=$(cat $MQTT_SONOFF_CONF_FILE | grep $PAR)
        if [ -z "$MATCH" ]; then
            echo "$i" >> $MQTT_SONOFF_CONF_FILE
        fi
    fi
done
