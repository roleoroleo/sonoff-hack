#!/bin/sh

DIR="none"

for I in 1
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "dir" ] ; then
        DIR="-a $VAL"
    fi
done

if [ "$DIR" != "none" ] ; then
    ptz $DIR
fi

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "}"
