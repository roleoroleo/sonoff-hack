#!/bin/sh

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"
MODEL_SUFFIX=$(cat /mnt/mmc/sonoff-hack/model)

#LOG_FILE="/mnt/mmc/wd_rtsp.log"
LOG_FILE="/dev/null"

IS_RUNNING=0

restart_rtsp()
{
    /mnt/mtd/ipc/app/avencode &
}

check_rtsp()
{
    IS_RUNNING=`ps | grep '/mnt/mtd/ipc/app/a[v]encode' | grep -v grep | grep -c ^`
}


echo "$(date +'%Y-%m-%d %H:%M:%S') - Starting RTSP watchdog..." >> $LOG_FILE

while true
do
    check_rtsp
    if [ $IS_RUNNING -eq 0 ]; then
        echo "$(date +'%Y-%m-%d %H:%M:%S') - Restarting RTSP service, running avencode process..." >> $LOG_FILE
        restart_rtsp
    fi
    sleep 10
done
