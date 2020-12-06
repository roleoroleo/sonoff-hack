#!/bin/sh

validateFile()
{
    case $1 in
        *[\'!\"@\#\$%\&^*\(\),:\;]* )
            echo "invalid";;
        *)
            echo $1;;
    esac
}

BASE64="no"
OUTPUT_FILE="none"

for I in 1 2
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "base64" ] ; then
        BASE64=$VAL
    elif [ "$CONF" == "file" ] ; then
        OUTPUT_FILE=$VAL
    fi
done

REDIRECT=""
if [ "$OUTPUT_FILE" != "none" ] ; then
    OUTPUT_FILE=$(validateFile "$OUTPUT_FILE")
    if [ "$OUTPUT_FILE" != "invalid" ]; then
        OUTPUT_DIR=$(cd "$(dirname "/mnt/mmc/sonoff-hack/www/alarm_record/$OUTPUT_FILE")"; pwd)
        OUTPUT_DIR=$(echo "$OUTPUT_DIR" | cut -c1-37)
        if [ "$OUTPUT_DIR" == "/mnt/mmc/sonoff-hack/www/alarm_record" ]; then
            REDIRECT="yes"
        fi
    fi
fi

if [ "$REDIRECT" == "yes" ] ; then
    if [ "$BASE64" == "no" ] ; then
        snapshot > /mnt/mmc/sonoff-hack/www/alarm_record/$OUTPUT_FILE
    elif [ "$BASE64" == "yes" ] ; then
        snapshot | base64 > /mnt/mmc/sonoff-hack/www/alarm_record/$OUTPUT_FILE
    fi
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "false"
    printf "}"
else
    if [ "$BASE64" == "no" ] ; then
        printf "Content-type: image/jpeg\r\n\r\n"
        snapshot
    elif [ "$BASE64" == "yes" ] ; then
        printf "Content-type: image/jpeg;base64\r\n\r\n"
        snapshot | base64
    fi
fi
