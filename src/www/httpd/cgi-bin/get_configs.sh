#!/bin/sh

get_conf_type()
{
    CONF="$(echo $QUERY_STRING | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

    if [ "$CONF" == "conf" ] ; then
        echo $VAL
    fi
}

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

. $SONOFF_HACK_PREFIX/www/cgi-bin/validate.sh

if ! $(validateQueryString $QUERY_STRING); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

printf "Content-type: application/json\r\n\r\n"

CONF_TYPE="$(get_conf_type)"
CONF_FILE=""

if [ "$CONF_TYPE" == "camera" ] ; then
    /mnt/mmc/sonoff-hack/www/cgi-bin/get_camera_settings.sh
elif [ "$CONF_TYPE" == "mqtt" ] ; then
    CONF_FILE="$SONOFF_HACK_PREFIX/etc/mqtt-sonoff.conf"
else
    CONF_FILE="$SONOFF_HACK_PREFIX/etc/$CONF_TYPE.conf"
fi

printf "{\n"

sed '/^#/d; /^$/d; s/\([^=]*\)=\(.*\)/"\1":"\2",/' "$CONF_FILE"

if [ "$CONF_TYPE" == "system" ] ; then
    printf "\"%s\":\"%s\",\n"  "HOSTNAME" "$(cat $SONOFF_HACK_PREFIX/etc/hostname | sed -r 's/\\/\\\\/g;s/"/\\"/g;')"
    TIMEZONE_N=$(sqlite3 /mnt/mtd/db/ipcsys.db "select c_param_value from t_sys_param where c_param_name='ZoneTimeName';")
    if [ ! -z $TIMEZONE_N ]; then
        let TIMEZONE_N=TIMEZONE_N-1
        TIMEZONE=$(sqlite3 /mnt/mtd/db/ipcsys.db "select * from t_zonetime_info LIMIT 1 OFFSET $TIMEZONE_N;")
        printf "\"%s\":\"%s\",\n"  "TIMEZONE" "$(echo $TIMEZONE | cut -d"|" -f1)"
    fi
fi

# Empty values to "close" the json
printf "\"%s\":\"%s\"\n"  "NULL" "NULL"

printf "}"
