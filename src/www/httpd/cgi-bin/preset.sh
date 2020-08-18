#!/bin/sh

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

ACTION="none"
NUM=-1

for I in 1 2
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "action" ] ; then
        ACTION="-a $VAL"
    elif [ "$CONF" == "num" ] ; then
        NUM="-n $VAL"
    fi
done

if [ $ACTION == "none" ]; then
    exit
fi
if [ $NUM -eq -1 ]; then
    exit
fi

$SONOFF_HACK_PREFIX/bin/ptz -f $SONOFF_HACK_PREFIX/etc/ptz_presets.conf $ACTION $NUM

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "$SONOFF_HACK_PREFIX/bin/ptz -f $SONOFF_HACK_PREFIX/etc/ptz_presets.conf $ACTION $NUM"
printf "}"
