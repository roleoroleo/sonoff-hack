#!/bin/sh

validateRecFile()
{
    if [ "${#1}" != "33" ] ; then
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

fbasename()
{
    echo ${1:0:$((${#1} - 4))}
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

FILE="none"
DIR="none"
ARC="none"

CONF="$(echo $QUERY_STRING | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

if [ "$CONF" == "file" ] ; then
    FILE="$VAL"
fi

validateRecFile $FILE

if [ "$DIR" == "none" ] || [ "$ARC" == "none" ] ; then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

BASE_NAME=$(fbasename "$ARC")
rm -f /mnt/mmc/alarm_record/$DIR/$BASE_NAME.mp4
rm -f /mnt/mmc/alarm_record/$DIR/$BASE_NAME.jpg

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
