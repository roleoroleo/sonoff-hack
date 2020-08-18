#!/bin/sh

DIR="none"
TIME=""

for I in 1 2
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "dir" ] ; then
        DIR="-a $VAL"
    elif [ "$CONF" == "time" ] ; then
        TIME="-t $VAL"
    fi
done

if [ "$DIR" != "none" ] ; then
    ptz $DIR $TIME
fi

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "}"
