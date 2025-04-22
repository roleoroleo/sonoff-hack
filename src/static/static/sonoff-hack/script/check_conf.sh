#!/bin/sh

SYSTEM_CONF_FILE="/mnt/mmc/sonoff-hack/etc/system.conf"
CAMERA_CONF_FILE="/mnt/mmc/sonoff-hack/etc/camera.conf"
MQTT_SONOFF_CONF_FILE="/mnt/mmc/sonoff-hack/etc/mqtt-sonoff.conf"

PARMS1="
HTTPD=yes
TELNETD=no
SSHD=yes
FTPD=yes
BUSYBOX_FTPD=no
MDNSD=yes
DISABLE_CLOUD=no
MQTT=no
SNAPSHOT=yes
SNAPSHOT_VIDEO=yes
ONVIF=yes
ONVIF_WSDD=yes
ONVIF_PROFILE=high
ONVIF_NETIF=eth0
ONVIF_FAULT_IF_UNKNOWN=0
ONVIF_SYNOLOGY_NVR=0
NTPD=yes
NTP_SERVER=pool.ntp.org
RTSP_PORT=554
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
FTP_PORT=21
SSH_PASSWORD=
SWAP_FILE=no
PTZ_PRESET_BOOT=default
CRONTAB=
SYSLOGD=no"

PARMS2=""

PARMS3="
MQTT_IP=0.0.0.0
MQTT_PORT=1883
MQTT_USER=
MQTT_PASSWORD=
MQTT_PREFIX=
TOPIC_BIRTH_WILL=status
TOPIC_MOTION=motion_detection
TOPIC_MOTION_IMAGE=motion_detection_image
MOTION_IMAGE_DELAY=0.5
BIRTH_MSG=online
WILL_MSG=offline
MOTION_START_MSG=motion_start
MOTION_STOP_MSG=motion_stop
HA_DISCOVERY=1
HA_CONF_PREFIX=homeassistant
HA_NAME_PREFIX=
MQTT_KEEPALIVE=120
MQTT_QOS=1
MQTT_RETAIN_BIRTH_WILL=1
MQTT_RETAIN_MOTION=0
MQTT_RETAIN_MOTION_IMAGE=0
MQTT_IPCSYS_DB=1"

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
