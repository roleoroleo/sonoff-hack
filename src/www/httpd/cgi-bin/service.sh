#!/bin/sh

CONF_FILE="etc/system.conf"
MQTT_CONF_FILE="etc/mqtt-sonoff.conf"

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"
START_STOP_SCRIPT=$SONOFF_HACK_PREFIX/script/service.sh

. $SONOFF_HACK_PREFIX/www/cgi-bin/validate.sh

if ! $(validateQueryString $QUERY_STRING); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

NAME="none"
ACTION="none"
PARAM1="none"
RES=""

for I in 1 2 3
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "name" ] ; then
        NAME="$VAL"
    elif [ "$CONF" == "action" ] ; then
        ACTION="$VAL"
    elif [ "$CONF" == "param1" ] ; then
        PARAM1="$VAL"
    fi
done

init_config

if [ "$ACTION" == "start" ] ; then
    if [ "$NAME" == "rtsp" ]; then
        $START_STOP_SCRIPT rtsp start
    elif [ "$NAME" == "onvif" ]; then
        $START_STOP_SCRIPT onvif start $PARAM1
    elif [ "$NAME" == "wsdd" ]; then
        $START_STOP_SCRIPT wsdd start
    elif [ "$NAME" == "ftpd" ]; then
        $START_STOP_SCRIPT ftpd start $PARAM1
    elif [ "$NAME" == "mqtt" ]; then
        $START_STOP_SCRIPT mqtt start
    elif [ "$NAME" == "all" ]; then
        $START_STOP_SCRIPT all start
    fi
elif [ "$ACTION" == "stop" ] ; then
    if [ "$NAME" == "rtsp" ]; then
        $START_STOP_SCRIPT rtsp stop
    elif [ "$NAME" == "onvif" ]; then
        $START_STOP_SCRIPT onvif stop
    elif [ "$NAME" == "wsdd" ]; then
        $START_STOP_SCRIPT wsdd stop
    elif [ "$NAME" == "ftpd" ]; then
        $START_STOP_SCRIPT ftpd stop
    elif [ "$NAME" == "mqtt" ]; then
        $START_STOP_SCRIPT mqtt stop
    elif [ "$NAME" == "all" ]; then
        $START_STOP_SCRIPT all stop
    fi
elif [ "$ACTION" == "status" ] ; then
    if [ "$NAME" == "rtsp" ]; then
        RES=$($START_STOP_SCRIPT rtsp status)
    elif [ "$NAME" == "onvif" ]; then
        RES=$($START_STOP_SCRIPT onvif status)
    elif [ "$NAME" == "wsdd" ]; then
        RES=$($START_STOP_SCRIPT wsdd status)
    elif [ "$NAME" == "ftpd" ]; then
        RES=$($START_STOP_SCRIPT ftpd status)
    elif [ "$NAME" == "mqtt" ]; then
        RES=$($START_STOP_SCRIPT mqtt status)
    fi
fi

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
if [ ! -z "$RES" ]; then
    printf "\"status\": \"$RES\"\n"
fi
printf "}"
