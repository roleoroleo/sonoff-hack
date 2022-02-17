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

CONF_LAST="CONF_LAST"

for I in 1 2 3 4
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if ! $(validateString $CONF); then
        printf "Content-type: application/json\r\n\r\n"
        printf "{\n"
        printf "\"%s\":\"%s\"\\n" "error" "true"
        printf "}"
        exit
    fi
    if ! $(validateString $VAL); then
        printf "Content-type: application/json\r\n\r\n"
        printf "{\n"
        printf "\"%s\":\"%s\"\\n" "error" "true"
        printf "}"
        exit
    fi

    if [ $CONF == $CONF_LAST ]; then
        continue
    fi
    CONF_LAST=$CONF

#    if [ "$CONF" == "switch_on" ] ; then
#        if [ "$VAL" == "no" ] ; then
#            ipc_cmd -t off
#        else
#            ipc_cmd -t on
#        fi
    if [ "$CONF" == "motion_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            sqlite3 /mnt/mtd/db/ipcsys.db "update t_mdarea set c_left=0,c_top=0,c_right=1920,c_bottom=1080,c_sensitivity=0,c_enable=0,c_name=\"P2P_SET\" where c_index=0;"
        else
            sqlite3 /mnt/mtd/db/ipcsys.db "update t_mdarea set c_left=0,c_top=0,c_right=1920,c_bottom=1080,c_sensitivity=25,c_enable=1,c_name=\"P2P_SET\" where c_index=0;"
        fi
    elif [ "$CONF" == "sensitivity" ] ; then
        if [ -z "$VAL" ]; then
            VAL=0
        fi
        sqlite3 /mnt/mtd/db/ipcsys.db "update t_mdarea set c_sensitivity=$VAL where c_index=0;"
    elif [ "$CONF" == "local_record" ] ; then
        if [ "$VAL" == "no" ] ; then
            sqlite3 /mnt/mtd/db/ipcsys.db "update t_record_plan set c_enabled=0 where c_recplan_no=1;"
        else
            sqlite3 /mnt/mtd/db/ipcsys.db "update t_record_plan set c_enabled=1 where c_recplan_no=1;"
        fi
#    elif [ "$CONF" == "baby_crying_detect" ] ; then
#        if [ "$VAL" == "no" ] ; then
#            ipc_cmd -b off
#        else
#            ipc_cmd -b on
#        fi
#    elif [ "$CONF" == "led" ] ; then
#        if [ "$VAL" == "no" ] ; then
#            ipc_cmd -l off
#        else
#            ipc_cmd -l on
#        fi
#    elif [ "$CONF" == "ir" ] ; then
#        if [ "$VAL" == "no" ] ; then
#            ipc_cmd -i off
#        else
#            ipc_cmd -i on
#        fi
    elif [ "$CONF" == "rotate" ] ; then
        if [ "$VAL" == "no" ] ; then
            sqlite3 /mnt/mtd/db/ipcsys.db "update t_sys_param set c_param_value=\"0\" where c_param_name=\"flip\";"
            sqlite3 /mnt/mtd/db/ipcsys.db "update t_sys_param set c_param_value=\"0\" where c_param_name=\"mirror\";"
        else
            sqlite3 /mnt/mtd/db/ipcsys.db "update t_sys_param set c_param_value=\"1\" where c_param_name=\"flip\";"
            sqlite3 /mnt/mtd/db/ipcsys.db "update t_sys_param set c_param_value=\"1\" where c_param_name=\"mirror\";"
        fi
    fi
    sleep 1
done

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
