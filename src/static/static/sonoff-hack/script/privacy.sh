#!/bin/sh

RES=""

start_rtsp()
{
    /mnt/mtd/ipc/app/rtspd >/dev/null &
}

stop_rtsp()
{
    killall rtspd
}

start_alarmserver()
{
    ps | grep 'AlarmServer' | grep -v 'grep' | awk '{ printf $1 }' |xargs kill -CONT
}

stop_alarmserver()
{
    ps | grep 'AlarmServer' | grep -v 'grep' | awk '{ printf $1 }' |xargs kill -SIGSTOP
}

start_ProcessGuard()
{
    ps | grep 'ProcessGuard' | grep -v 'grep' | awk '{ printf $1 }' |xargs kill -CONT 2> /dev/null
}

stop_ProcessGuard()
{
    ps | grep 'ProcessGuard' | grep -v 'grep' | awk '{ printf $1 }' |xargs kill -SIGSTOP 2> /dev/null
}

if [ $# -ne 1 ]; then
    exit
fi

if [ "$1" == "on" ] || [ "$1" == "yes" ]; then
    if [ ! -f /tmp/privacy ]; then
        touch /tmp/privacy
        touch /tmp/snapshot.disabled
        stop_ProcessGuard
        stop_rtsp
        stop_alarmserver
    fi
elif [ "$1" == "off" ] || [ "$1" == "no" ]; then
    if [ -f /tmp/privacy ]; then
        rm -f /tmp/snapshot.disabled
        start_ProcessGuard
        start_alarmserver
        start_rtsp
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
