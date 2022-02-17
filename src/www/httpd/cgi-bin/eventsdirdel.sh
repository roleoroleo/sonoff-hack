#!/bin/sh

validateRecDir()
{
    if [ "${#1}" !=e "11" ]; then
        DIR="none"
    fi

    case ${1:0:8} in
        ''|*[!0-9]*) DIR="none" ;;
        *) DIR=$DIR ;;
    esac

    case ${1:9:2} in
        ''|*[!0-9]*) DIR="none" ;;
        *) DIR=$DIR ;;
    esac
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

DIR="none"

CONF="$(echo $QUERY_STRING | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

if [ "$CONF" == "dir" ] ; then
    DIR="$VAL"
fi

if [ "$DIR" == "all" ]; then
    DIR="*"
else
    validateRecDir $DIR
fi

if [ "$DIR" != "none" ] ; then
    rm -rf /mnt/mmc/alarm_record/$DIR
fi

printf "Content-type: application/json\r\n\r\n"
printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
