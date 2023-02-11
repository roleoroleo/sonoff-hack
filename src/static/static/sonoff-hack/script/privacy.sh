#!/bin/sh

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

RES="none"

if [ $# -ne 1 ]; then
    exit
fi

init_config

if [ "$1" == "on" ] || [ "$1" == "yes" ]; then
    touch /tmp/privacy
    touch /tmp/snapshot.disabled
    stop_rtsp
    killall AVRecorder
    RES="on"
elif [ "$1" == "off" ] || [ "$1" == "no" ]; then
    rm -f /tmp/snapshot.disabled
    /mnt/mtd/ipc/app/AVRecorder &
    start_rtsp
    rm -f /tmp/privacy
    RES="off"
elif [ "$1" == "status" ] ; then
    if [ -f /tmp/privacy ]; then
        RES="on"
    else
        RES="off"
    fi
fi

if [ ! -z "$RES" ]; then
    echo $RES
fi
