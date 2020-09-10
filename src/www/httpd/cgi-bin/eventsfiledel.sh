#!/bin/sh

validateFile()
{
    if [ ${#1} != 33 ] ; then
        DIR="none"
        ARC="none"
    fi

    case ${1:0:8} in
        ''|*[!0-9]*) DIR="none" ;;
        *) DIR=${1:0:8} ;;
    esac

    case ${1:9:2} in
        ''|*[!0-9]*) DIR="none" ;;
        *) DIR=$DIR/${1:9:2} ;;
    esac

    if [ ${1:12:3} != "ARC" } ; then
        ARC="none"
    fi

    case ${1:15:14} in
        ''|*[!0-9]*) ARC="none" ;;
        *) ARC="ARC${1:15:14}.mp4" ;;
    esac
}

case $QUERY_STRING in
    *[\'!\"@\#\$%^*\(\)_+,:\;]* ) exit;;
esac

FILE="none"
DIR="none"
ARC="none"

CONF="$(echo $QUERY_STRING | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

if [ "$CONF" == "file" ] ; then
    FILE="$VAL"
fi

validateFile $FILE

if [ "$DIR" != "none" ] && [ "$ARC" != "none" ] ; then
    rm -f /mnt/mmc/alarm_record/$DIR/$ARC
fi

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "}"
