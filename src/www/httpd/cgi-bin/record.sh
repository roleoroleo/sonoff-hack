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

NAME="none"

CONF="$(echo $QUERY_STRING | cut -d'&' -f1 | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'&' -f1 | cut -d'=' -f2)"

if [ "$CONF" == "name" ] ; then
    NAME="$VAL"
fi

if ! $(validateBaseName $NAME); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

if [ "$NAME" == "none" ] ; then
    NAME=$(mktemp -u -p alarm_record/)
    FULL_NAME="/mnt/mmc/sonoff-hack/www/"$NAME
else
    NAME="alarm_record/$NAME"
    FULL_NAME="/mnt/mmc/sonoff-hack/www/"$NAME
fi

if [ ${NAME##*\.} != "mp4" ]; then
    NAME=$NAME.mp4
    FULL_NAME=$FULL_NAME.mp4
fi

record -f $FULL_NAME

printf "Content-type: application/json\r\n\r\n"
printf "{\n"
printf "\"name\":\"$FULL_NAME\",\n"
printf "\"link\":\"$NAME\"\n"
printf "}\n"
