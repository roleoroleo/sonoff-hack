#!/bin/sh

RES=""

if [ $# -ne 1 ]; then
    exit
fi

if [ "$1" == "on" ] || [ "$1" == "yes" ]; then
    if [ ! -f /tmp/privacy ]; then
        touch /tmp/privacy
        touch /tmp/snapshot.disabled
        killall rtspd
        killall AVRecorder
    fi
elif [ "$1" == "off" ] || [ "$1" == "no" ]; then
    if [ -f /tmp/privacy ]; then
        rm -f /tmp/snapshot.disabled
        /mnt/mtd/ipc/app/AVRecorder >/dev/null &
        /mnt/mtd/ipc/app/rtspd >/dev/null &
        rm -f /tmp/privacy
    fi
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
