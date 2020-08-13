#!/bin/sh

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/mmc/sonoff-hack/lib
export PATH=$PATH:/mnt/mmc/sonoff-hack/bin:/mnt/mmc/sonoff-hack/sbin:/mnt/mmc/sonoff-hack/usr/bin:/mnt/mmc/sonoff-hack/usr/sbin

NAME="$(echo $QUERY_STRING | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

if [ "$NAME" != "get" ] ; then
    exit
fi

if [ "$VAL" == "info" ] ; then
    printf "Content-type: application/json\r\n\r\n"

    FW_VERSION=`cat /mnt/mmc/sonoff-hack/version`
    LATEST_FW=`wget -O -  https://api.github.com/repos/roleoroleo/sonoff-hack/releases/latest 2>&1 | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/'`

    printf "{\n"
    printf "\"%s\":\"%s\",\n" "fw_version"      "$FW_VERSION"
    printf "\"%s\":\"%s\"\n" "latest_fw"       "$LATEST_FW"
    printf "}"

elif [ "$VAL" == "upgrade" ] ; then

    FREE_SD=$(df /mnt/mmc/ | grep mmc | awk '{print $4}')
    if [ -z "$FREE_SD" ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "No SD detected."
        exit
    fi

    if [ $FREE_SD -lt 100000 ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "No space left on SD."
        exit
    fi

    rm -rf /mnt/mmc/.fw_upgrade
    mkdir -p /mnt/mmc/.fw_upgrade
    cd /mnt/mmc/.fw_upgrade

    MODEL=$(cat /mnt/mtd/ipc/cfg/config_cst.cfg | grep model | cut -d'=' -f2 | cut -d'"' -f2)
    FW_VERSION=`cat /mnt/mmc/sonoff-hack/version`
    LATEST_FW=`wget -O -  https://api.github.com/repos/roleoroleo/sonoff-hack/releases/latest 2>&1 | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/'`
    if [ "$FW_VERSION" == "$LATEST_FW" ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "No new firmware available."
        exit
    fi

    wget https://github.com/roleoroleo/sonoff-hack/releases/download/$LATEST_FW/${MODEL}_${LATEST_FW}.tgz
    if [ ! -f ${MODEL}_${LATEST_FW}.tgz ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "Unable to download firmware file."
        exit
    fi

    tar zxvf ${MODEL}_${LATEST_FW}.tgz
    rm ${MODEL}_${LATEST_FW}.tgz
    mv -f * ..
    cp -f $SONOFF_HACK_PREFIX/etc/*.conf .
    if [ -f $SONOFF_HACK_PREFIX/etc/hostname ]; then
        cp -f $SONOFF_HACK_PREFIX/etc/hostname .
    fi

    # Report the status to the caller
    printf "Content-type: text/html\r\n\r\n"
    printf "Download completed, rebooting and upgrading."

    sync
    sync
    sync
    reboot
fi
