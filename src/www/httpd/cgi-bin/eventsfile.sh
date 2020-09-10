#!/bin/sh

printf "Content-type: application/json\r\n\r\n"

case $QUERY_STRING in
    *[\'!\"@\#\$%^*\(\)_+.,:\;]* ) exit;;
esac

CONF="$(echo $QUERY_STRING | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

if [ "$CONF" == "dirname" ]; then
     DIR=$VAL
fi

if [ ${#DIR} == 11 ]; then
#    DIR="${DIR:0:8}/${DIR:8:2}"

    printf "{\"date\":\"${DIR:0:4}-${DIR:4:2}-${DIR:6:2}\",\n"
    printf "\"records\":[\n"

    COUNT=`ls -r /mnt/mmc/sonoff-hack/www/alarm_record/$DIR | grep mp4 -c`
    IDX=1
    for f in `ls -r /mnt/mmc/sonoff-hack/www/alarm_record/$DIR | grep mp4`; do
        if [ ${#f} == 21 ]; then
            printf "{\n"
            printf "\"%s\":\"%s\",\n" "time" "Time: ${f:11:2}:${f:13:2}"
            printf "\"%s\":\"%s\"\n" "filename" "$f"
            if [ "$IDX" == "$COUNT" ]; then
                printf "}\n"	
            else
                printf "},\n"
            fi
            IDX=$(($IDX+1))
        fi
    done
    printf "]}\n"
fi
