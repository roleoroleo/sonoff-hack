#!/bin/sh

validateFile()
{
    case ${1} in
        ''|*[\\\/\ ]*) NAME="invalid" ;;
        *) NAME=$NAME ;;
    esac
}

NAME="none"

for I in 1
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "name" ] ; then
        NAME="$VAL"
    fi
done

validateFile "$NAME"

if [ "$NAME" == "invalid" ] ; then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"error\"=\"Invalid name\"\n"
    printf "}\n"
    exit
elif [ "$NAME" == "none" ] ; then
    NAME=$(mktemp -u -p /mnt/mmc/)
else
    NAME="/mnt/mmc/$NAME"
fi

if [ ${NAME##*\.} != "mp4" ]; then
    NAME=$NAME.mp4
fi

record -f $NAME

printf "Content-type: application/json\r\n\r\n"
printf "{\n"
printf "\"name\"=\"$NAME\"\n"
printf "\"link\"=\"$NAME\"\n"
printf "}\n"
