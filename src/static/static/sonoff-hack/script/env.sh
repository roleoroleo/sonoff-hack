#!/bin/sh

CONF_FILE="etc/system.conf"

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/mmc/sonoff-hack/lib
export PATH=$PATH:/mnt/mmc/sonoff-hack/bin:/mnt/mmc/sonoff-hack/sbin:/mnt/mmc/sonoff-hack/usr/bin:/mnt/mmc/sonoff-hack/usr/sbin

get_config()
{
    key=$1
    grep -w $1 $SONOFF_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2
}

TZ_CONF=$(get_config TIMEZONE)

if [ ! -z "$TZ_CONF" ]; then
    export TZ=$TZ_CONF
fi
