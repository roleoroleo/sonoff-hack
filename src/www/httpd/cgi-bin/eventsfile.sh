#!/bin/sh

validateRecDir()
{
    if [ "${#1}" != "11" ]; then
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

CONF="$(echo $QUERY_STRING | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

if [ "$CONF" == "dirname" ]; then
     DIR=$VAL
fi

#DIR="${DIR:0:8}/${DIR:8:2}"
validateRecDir $DIR

if [ "$DIR" == "none" ] ; then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

printf "Content-type: application/json\r\n\r\n"

printf "{"
printf "\"%s\":\"%s\",\\n" "error" "false"
printf "\"date\":\"${DIR:0:4}-${DIR:4:2}-${DIR:6:2}\",\n"
printf "\"records\":[\n"

COUNT=`ls -r /mnt/mmc/sonoff-hack/www/alarm_record/$DIR | grep mp4 -c`
IDX=1
for f in `ls -r /mnt/mmc/sonoff-hack/www/alarm_record/$DIR | grep mp4`; do
    if [ ${#f} == 21 ]; then
        base_name=$(fbasename "$f")
        if [ -f /mnt/mmc/sonoff-hack/www/alarm_record/$DIR/$base_name.jpg ] && [ -s /mnt/mmc/sonoff-hack/www/alarm_record/$DIR/$base_name.jpg ]; then
            thumbbasename="$base_name.jpg"
        else
            thumbbasename=""
        fi
        printf "{\n"
        printf "\"%s\":\"%s\",\n" "time" "Time: ${f:11:2}:${f:13:2}"
        printf "\"%s\":\"%s\",\n" "filename" "$f"
        printf "\"%s\":\"%s\"\n" "thumbfilename" "$thumbbasename"
        if [ "$IDX" == "$COUNT" ]; then
            printf "}\n"
        else
            printf "},\n"
        fi
        IDX=$(($IDX+1))
    fi
done

printf "]}\n"
