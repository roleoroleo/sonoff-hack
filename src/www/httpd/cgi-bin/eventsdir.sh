#!/bin/sh

printf "Content-type: application/json\r\n\r\n"

printf "{\"records\":[\n"

COUNT=`ls -r /mnt/mmc/sonoff-hack/www/alarm_record/* | grep -v alarm | grep -v ^$ | grep ^ -c`
IDX=1
for f in `ls -r /mnt/mmc/sonoff-hack/www/alarm_record | grep ^`; do
    if [ ${#f} == 8 ]; then
        for f2 in `ls -r /mnt/mmc/sonoff-hack/www/alarm_record/$f | grep ^`; do
            if [ ${#f2} == 2 ]; then
                printf "{\n"
                printf "\"%s\":\"%s\",\n" "datetime" "Date: ${f:0:4}-${f:4:2}-${f:6:2} Time: $f2:00"
                printf "\"%s\":\"%s\"\n" "dirname" "$f/$f2"
                if [ "$IDX" == "$COUNT" ]; then
                    printf "}\n"
                else
                    printf "},\n"
                fi
                IDX=$(($IDX+1))
            fi
        done
    fi
done

printf "]}\n"
