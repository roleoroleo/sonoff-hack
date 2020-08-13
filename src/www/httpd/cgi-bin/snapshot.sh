#!/bin/sh

BASE64="no"

for I in 1
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "base64" ] ; then
        BASE64=$VAL
    fi
done

if [ "$BASE64" == "no" ] ; then
    printf "Content-type: image/jpeg\r\n\r\n"
    snapshot
elif [ "$BASE64" == "yes" ] ; then
    printf "Content-type: image/jpeg;base64\r\n\r\n"
    snapshot | base64
fi
