#!/bin/sh

# for backwards compatibility

CONF_FILE="etc/system.conf"
SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

get_config()
{
    key=$1
    grep -w $1 $SONOFF_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2
}

init_config()
{
    if [[ x$(get_config USERNAME) != "x" ]] ; then
        USERNAME=$(get_config USERNAME)
        PASSWORD=$(get_config PASSWORD)
    fi

    case $(get_config RTSP_PORT) in
        ''|*[!0-9]*) RTSP_PORT=554 ;;
        *) RTSP_PORT=$(get_config RTSP_PORT) ;;
    esac

    if [[ $RTSP_PORT != "554" ]] ; then
        D_RTSP_PORT=:$RTSP_PORT
    fi
}

start_rtsp()
{
    /mnt/mtd/ipc/app/rtspd >/dev/null &
}

stop_rtsp()
{
    killall rtspd
}


. $SONOFF_HACK_PREFIX/www/cgi-bin/validate.sh

if ! $(validateQueryString $QUERY_STRING); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

VALUE="none"
RES="none"

CONF="$(echo $QUERY_STRING | cut -d'&' -f1 | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'&' -f1 | cut -d'=' -f2)"

if [ "$CONF" == "value" ] ; then
    VALUE="$VAL"
fi

init_config

if [ "$VALUE" == "on" ] ; then
    touch /tmp/privacy
    touch /tmp/snapshot.disabled
    stop_rtsp
    killall AVRecorder
    RES="on"
elif [ "$VALUE" == "off" ] ; then
    rm -f /tmp/snapshot.disabled
    /mnt/mtd/ipc/app/AVRecorder &
    start_rtsp
    rm -f /tmp/privacy
    RES="off"
elif [ "$VALUE" == "status" ] ; then
    if [ -f /tmp/privacy ]; then
        RES="on"
    else
        RES="off"
    fi
fi

printf "Content-type: application/json\r\n\r\n"
printf "{\n"
if [ ! -z "$RES" ]; then
    printf "\"status\": \"$RES\",\n"
fi
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
