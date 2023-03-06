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

for I in 1 2 3 4 5 6
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

    if [ "$CONF" == "switch_on" ] ; then
        if [ "$VAL" == "no" ] ; then
            $SONOFF_HACK_PREFIX/script/privacy.sh on
        else
            $SONOFF_HACK_PREFIX/script/privacy.sh off
        fi
    elif [ "$CONF" == "motion_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
update t_mdarea set c_left=0,c_top=0,c_right=1920,c_bottom=1080,c_sensitivity=0,c_enable=0,c_name="P2P_SET" where c_index=0;
EOF
        else
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
update t_mdarea set c_left=0,c_top=0,c_right=1920,c_bottom=1080,c_sensitivity=25,c_enable=1,c_name="P2P_SET" where c_index=0;
EOF
        fi
    elif [ "$CONF" == "sensitivity" ] ; then
        if [ -z "$VAL" ]; then
            VAL=0
        elif [ "$VAL" == "off" ]; then
            VAL=0
        elif [ "$VAL" == "low" ]; then
            VAL=25
        elif [ "$VAL" == "medium" ]; then
            VAL=50
        elif [ "$VAL" == "high" ]; then
            VAL=75
        fi
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
update t_mdarea set c_sensitivity=$VAL where c_index=0;
EOF
    elif [ "$CONF" == "local_record" ] ; then
        if [ "$VAL" == "no" ] ; then
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
update t_record_plan set c_enabled=0 where c_recplan_no=1;
EOF
        else
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
update t_record_plan set c_enabled=1 where c_recplan_no=1;
EOF
        fi
    elif [ "$CONF" == "ir" ] ; then
        if [ -z "$VAL" ]; then
            VAL=2
        elif [ "$VAL" == "auto" ]; then
            VAL=2
        elif [ "$VAL" == "on" ]; then
            VAL=0
        elif [ "$VAL" == "off" ]; then
            VAL=1
        fi
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
update t_sys_param set c_param_value=$VAL where c_param_name="InfraredLamp";
EOF
    elif [ "$CONF" == "rotate" ] ; then
        if [ "$VAL" == "no" ] ; then
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
update t_sys_param set c_param_value="0" where c_param_name="flip";
EOF
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
update t_sys_param set c_param_value="0" where c_param_name="mirror";
EOF
        else
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
update t_sys_param set c_param_value="1" where c_param_name="flip";
EOF
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
update t_sys_param set c_param_value="1" where c_param_name="mirror";
EOF
        fi
    fi
    sleep 1
done

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
