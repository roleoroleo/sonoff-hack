#!/bin/sh

urldecode(){
    echo -e "$(sed 's/+/ /g;s/%\(..\)/\\x\1/g;')"
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

ACTION="none"
NUM=-1
NAME="none"

for I in 1 2 3
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "action" ] ; then
        ACTION="$VAL"
    elif [ "$CONF" == "num" ] ; then
        if $(validateNumber $VAL); then
            NUM="$VAL"
        fi
    elif [ "$CONF" == "name" ] ; then
        VAL=$(echo "$VAL" | urldecode)
        NAME="$VAL"
    fi
done

if [ "$ACTION" != "go_preset" ] && [ "$ACTION" != "set_preset" ]; then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

if [ $NUM -eq -1 ]; then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

if [ "$ACTION" == "set_preset" ] && [ "$NAME" == "none" ]; then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

ARG_ACTION="-a $ACTION"
ARG_NUM="-n $NUM"
if [ "$ACTION" == "set_preset" ] && [ "$NAME" == "" ]; then
    # Remove entry
    ARG_NAME="-c"
else
    ARG_NAME="-e $NAME"
fi

$SONOFF_HACK_PREFIX/bin/ptz -f $SONOFF_HACK_PREFIX/etc/ptz_presets.conf $ARG_ACTION $ARG_NUM $ARG_NAME

printf "Content-type: application/json\r\n\r\n"
printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
