#!/bin/sh

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/mmc/sonoff-hack/lib
export PATH=$PATH:/mnt/mmc/sonoff-hack/bin:/mnt/mmc/sonoff-hack/sbin:/mnt/mmc/sonoff-hack/usr/bin:/mnt/mmc/sonoff-hack/usr/sbin

SAVE_VIDEO_ON_MOTION=$(sqlite3 /mnt/mtd/db/ipcsys.db "select c_enable from t_mdarea where c_index=0;")
if [ "$SAVE_VIDEO_ON_MOTION" == "0" ]; then
    SAVE_VIDEO_ON_MOTION="no"
else
    SAVE_VIDEO_ON_MOTION="yes"
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
printf "\"%s\":\"%s\",\n" "SAVE_VIDEO_ON_MOTION"    "$SAVE_VIDEO_ON_MOTION"
printf "\"%s\":\"%s\",\n" "SENSITIVITY"             "$SENSITIVITY"
printf "\"%s\":\"%s\" \n" "ROTATE"                  "$ROTATE"
printf "}"
