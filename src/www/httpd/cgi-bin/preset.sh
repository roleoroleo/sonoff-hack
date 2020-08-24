#!/bin/sh

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

urldecode(){
  echo -e "$(sed 's/+/ /g;s/%\(..\)/\\x\1/g;')"
}

ACTION="none"
NUM=-1
NAME=""

for I in 1 2 3
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "action" ] ; then
        ACTION="-a $VAL"
    elif [ "$CONF" == "num" ] ; then
        NUM="-n $VAL"
    elif [ "$CONF" == "name" ] ; then
        VAL=$(echo "$VAL" | urldecode)
        NAME="-e $VAL"
    fi
done

if [ $ACTION == "none" ]; then
    exit
fi
if [ $NUM -eq -1 ]; then
    exit
fi

$SONOFF_HACK_PREFIX/bin/ptz -f $SONOFF_HACK_PREFIX/etc/ptz_presets.conf $ACTION $NUM $NAME

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "}"
