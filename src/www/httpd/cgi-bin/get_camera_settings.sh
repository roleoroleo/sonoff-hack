#!/bin/sh

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/mmc/sonoff-hack/lib
export PATH=$PATH:/mnt/mmc/sonoff-hack/bin:/mnt/mmc/sonoff-hack/sbin:/mnt/mmc/sonoff-hack/usr/bin:/mnt/mmc/sonoff-hack/usr/sbin

MOTION_DETECTION=$(sqlite3 /mnt/mtd/db/ipcsys.db "select c_enable from t_mdarea where c_index=0;")
if [ "$MOTION_DETECTION" == "0" ]; then
    MOTION_DETECTION="no"
else
    MOTION_DETECTION="yes"
fi
SENSITIVITY=$(sqlite3 /mnt/mtd/db/ipcsys.db "select c_sensitivity from t_mdarea where c_index=0;")
ROTATE=$(sqlite3 /mnt/mtd/db/ipcsys.db "select c_param_value from t_sys_param where c_param_name=\"mirror\";")
if [ "$ROTATE" == "0" ]; then
    ROTATE="no"
else
    ROTATE="yes"
fi

printf "Content-type: application/json\r\n\r\n"
printf "{\n"
printf "\"%s\":\"%s\",\n" "MOTION_DETECTION"        "$MOTION_DETECTION"
printf "\"%s\":\"%s\",\n" "SENSITIVITY"             "$SENSITIVITY"
printf "\"%s\":\"%s\" \n" "ROTATE"                  "$ROTATE"
printf "}"
