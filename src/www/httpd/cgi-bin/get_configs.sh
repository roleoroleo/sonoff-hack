#!/bin/sh

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

get_conf_type()
{
    CONF="$(echo $QUERY_STRING | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

    if [ "$CONF" == "conf" ] ; then
        echo $VAL
    fi
}

printf "Content-type: application/json\r\n\r\n"

CONF_TYPE="$(get_conf_type)"
CONF_FILE=""

if [ "$CONF_TYPE" == "mqtt" ] ; then
    CONF_FILE="$SONOFF_HACK_PREFIX/etc/mqtt-sonoff.conf"
else
    CONF_FILE="$SONOFF_HACK_PREFIX/etc/$CONF_TYPE.conf"
fi

printf "{\n"

while IFS= read -r LINE ; do
    if [ ! -z "$LINE" ] ; then
        if [ "$LINE" == "${LINE#\#}" ] ; then # skip comments
#            printf "\"%s\",\n" $(echo "$LINE" | sed -r 's/=/":"/g') # Format to json and replace = with ":"
            echo -n "\""
            echo -n "$LINE" | sed -r 's/=/":"/g;'
            echo "\","
        fi
    fi
done < "$CONF_FILE"

if [ "$CONF_TYPE" == "system" ] ; then
    printf "\"%s\":\"%s\",\n"  "HOSTNAME" "$(cat $SONOFF_HACK_PREFIX/etc/hostname)"
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
