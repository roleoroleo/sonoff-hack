#!/bin/sh

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

urldecode(){
  echo -e "$(sed 's/+/ /g;s/%\(..\)/\\x\1/g;')"
}

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
        NUM="$VAL"
    elif [ "$CONF" == "name" ] ; then
        VAL=$(echo "$VAL" | urldecode)
        NAME="$VAL"
    fi
done

if [ "$ACTION" != "go_preset" ] && [ "$ACTION" != "set_preset" ]; then
    exit
fi

if [ $NUM -eq -1 ]; then
    exit
fi

if [ "$ACTION" == "set_preset" ] && [ "$NAME" == "none" ]; then
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
echo "$SONOFF_HACK_PREFIX/bin/ptz -f $SONOFF_HACK_PREFIX/etc/ptz_presets.conf $ARG_ACTION $ARG_NUM $ARG_NAME"

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "}"
