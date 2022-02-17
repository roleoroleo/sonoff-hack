#!/bin/sh

export PATH=$PATH:/mnt/mmc/sonoff-hack/bin:/mnt/mmc/sonoff-hack/sbin:/mnt/mmc/sonoff-hack/usr/bin:/mnt/mmc/sonoff-hack/usr/sbin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/mmc/sonoff-hack/lib

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

. $SONOFF_HACK_PREFIX/www/cgi-bin/validate.sh

if ! $(validateQueryString $QUERY_STRING); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

NAME="$(echo $QUERY_STRING | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

if [ "$NAME" != "get" ] ; then
    exit
fi

if [ "$VAL" == "info" ] ; then
    printf "Content-type: application/json\r\n\r\n"

    FW_VERSION=`cat /mnt/mmc/sonoff-hack/version`
    LATEST_FW=`/mnt/mmc/sonoff-hack/usr/bin/wget -O -  https://api.github.com/repos/roleoroleo/sonoff-hack/releases/latest 2>&1 | grep '"tag_name":' | sed -r 's/.*"([^"]+)".*/\1/'`

    printf "{\n"
    printf "\"%s\":\"%s\",\n" "error" "false"
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

    # Clean old upgrades
    rm -rf /mnt/mmc/.fw_upgrade
    rm -rf /mnt/mmc/.fw_upgrade.conf

    mkdir -p /mnt/mmc/.fw_upgrade
    mkdir -p /mnt/mmc/.fw_upgrade.conf
    cd /mnt/mmc/.fw_upgrade

    #MODEL=$(cat /mnt/mtd/ipc/cfg/config_cst.cfg | grep model | cut -d'=' -f2 | cut -d'"' -f2)
    MODEL="$(cat ${SONOFF_HACK_PREFIX}/model)"
    FW_VERSION=`cat /mnt/mmc/sonoff-hack/version`
    if [ -f /mnt/mmc/${MODEL}_x.x.x.tgz ]; then
        mv /mnt/mmc/${MODEL}_x.x.x.tgz /mnt/mmc/.fw_upgrade/${MODEL}_x.x.x.tgz
        LATEST_FW="x.x.x"
    else
        LATEST_FW=`/mnt/mmc/sonoff-hack/usr/bin/wget -O -  https://api.github.com/repos/roleoroleo/sonoff-hack/releases/latest 2>&1 | grep '"tag_name":' | sed -r 's/.*"([^"]+)".*/\1/'`
        if [ "$FW_VERSION" == "$LATEST_FW" ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "No new firmware available."
            exit
        fi

        $SONOFF_HACK_PREFIX/usr/bin/wget https://github.com/roleoroleo/sonoff-hack/releases/download/$LATEST_FW/${MODEL}_${LATEST_FW}.tgz
        if [ ! -f ${MODEL}_${LATEST_FW}.tgz ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "Unable to download firmware file."
            exit
        fi
    fi

    # Backup configuration
    cp -f $SONOFF_HACK_PREFIX/etc/* /mnt/mmc/.fw_upgrade.conf/
    rm /mnt/mmc/.fw_upgrade.conf/*.tar.gz

    # Prepare new hack
    $SONOFF_HACK_PREFIX/bin/tar zxvf ${MODEL}_${LATEST_FW}.tgz
    rm ${MODEL}_${LATEST_FW}.tgz
    mkdir -p /mnt/mmc/.fw_upgrade/sonoff-hack/etc
    cp -rf /mnt/mmc/.fw_upgrade.conf/* /mnt/mmc/.fw_upgrade/sonoff-hack/etc/
    rm -rf /mnt/mmc/.fw_upgrade.conf


    # Report the status to the caller
    printf "Content-type: text/html\r\n\r\n"
    printf "Upgrade completed, rebooting."

    sync
    sync
    sync
    sleep 1
    reboot
fi
