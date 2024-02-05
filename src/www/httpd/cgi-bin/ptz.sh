#!/bin/sh

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

. $SONOFF_HACK_PREFIX/www/cgi-bin/validate.sh

if ! $(validateQueryString $QUERY_STRING); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

DIR="none"
TIME="0.5"

for I in 1 2
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "dir" ] ; then
        if [ -f /tmp/.mirror ]; then
            if [ "$VAL" == "right" ]; then
                VAL="left"
            elif [ "$VAL" == "left" ]; then
                VAL="right"
            elif [ "$VAL" == "up" ]; then
                VAL="down"
            elif [ "$VAL" == "down" ]; then
                VAL="up"
            fi
        fi
        DIR="-a $VAL"
    elif [ "$CONF" == "time" ] ; then
        TIME="$VAL"
    fi
done

if ! $(validateString $DIR); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\",\\n" "error" "true"
    printf "\"%s\":\"%s\"\\n" "description" "wrong dir parameter"
    printf "}"
    exit
fi

if [ ! -z $TIME ]; then
    if ! $(validateNumber $TIME); then
        printf "Content-type: application/json\r\n\r\n"
        printf "{\n"
        printf "\"%s\":\"%s\",\\n" "error" "true"
        printf "\"%s\":\"%s\"\\n" "description" "wrong time parameter"
        printf "}"
        exit
    fi

    # convert time to milliseconds
    TIME=$(awk "BEGIN {print $TIME*1000}")
    TIME="-t $TIME"
fi

if [ "$DIR" != "none" ] ; then
    ptz $DIR $TIME
fi

printf "Content-type: application/json\r\n\r\n"
printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
