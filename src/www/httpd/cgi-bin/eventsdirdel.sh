#!/bin/sh

validateDir()
{
    case ${1:0:8} in
        ''|*[!0-9]*) DIR="none" ;;
        *) DIR=$DIR ;;
    esac

    case ${1:9:2} in
        ''|*[!0-9]*) DIR="none" ;;
        *) DIR=$DIR ;;
    esac
}

case $QUERY_STRING in
    *[\'!\"@\#\$%^*\(\)_+.,:\;]* ) exit;;
esac

DIR="none"

CONF="$(echo $QUERY_STRING | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

if [ "$CONF" == "dir" ] ; then
    DIR="$VAL"
fi

if [ "$DIR" == "all" ]; then
    DIR="*"
else
    validateDir $DIR
fi

if [ "$DIR" != "none" ] ; then
    rm -rf /mnt/mmc/alarm_record/$DIR
fi

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "}"
