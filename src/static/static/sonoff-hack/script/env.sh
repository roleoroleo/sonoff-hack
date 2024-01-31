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

TIMEZONE_N=$(sqlite3 /mnt/mtd/db/ipcsys.db "select c_param_value from t_sys_param where c_param_name='ZoneTimeName';")
if [ ! -z $TIMEZONE_N ]; then
    let TIMEZONE_N=TIMEZONE_N-1
    TIMEZONE=$(sqlite3 /mnt/mtd/db/ipcsys.db "select * from t_zonetime_info LIMIT 1 OFFSET $TIMEZONE_N;")
    export TZ=$(echo $TIMEZONE | cut -d"|" -f1)
fi
