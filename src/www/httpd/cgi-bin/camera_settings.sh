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
MOTION="NONE"
SENSITIVITY="NONE"

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
        if [ "$VAL" == "yes" ] ; then
            $SONOFF_HACK_PREFIX/bin/ipc_cmd -t on
        elif [ "$VAL" == "no" ] ; then
            $SONOFF_HACK_PREFIX/bin/ipc_cmd -t off
        fi
    elif [ "$CONF" == "motion_detection" ] ; then
        if [ "$VAL" == "yes" ] ; then
            MOTION="YES"
        elif [ "$VAL" == "no" ] ; then
            MOTION="NO"
        fi
    elif [ "$CONF" == "sensitivity" ] ; then
        if [ "$VAL" == "low" ]; then
            SENSITIVITY="LOW"
        elif [ "$VAL" == "medium" ]; then
            SENSITIVITY="MEDIUM"
        elif [ "$VAL" == "high" ]; then
            SENSITIVITY="HIGH"
        fi
    elif [ "$CONF" == "local_record" ] ; then
        if [ "$VAL" == "yes" ] ; then
            $SONOFF_HACK_PREFIX/bin/ipc_cmd -l on
        elif [ "$VAL" == "no" ] ; then
            $SONOFF_HACK_PREFIX/bin/ipc_cmd -l off
        fi
    elif [ "$CONF" == "ir" ] ; then
        if [ "$VAL" == "auto" ]; then
            $SONOFF_HACK_PREFIX/bin/ipc_cmd -i auto
        elif [ "$VAL" == "on" ]; then
            $SONOFF_HACK_PREFIX/bin/ipc_cmd -i on
        elif [ "$VAL" == "off" ]; then
            $SONOFF_HACK_PREFIX/bin/ipc_cmd -i off
        fi
    elif [ "$CONF" == "rotate" ] ; then
        if [ "$VAL" == "yes" ] ; then
            $SONOFF_HACK_PREFIX/bin/ipc_cmd -r on
        elif [ "$VAL" == "no" ] ; then
            $SONOFF_HACK_PREFIX/bin/ipc_cmd -r off
        fi
    fi
    sleep 0.5
done

if [ "$MOTION" == "NO" ] ; then
    $SONOFF_HACK_PREFIX/bin/ipc_cmd -m off
elif [ "$MOTION" == "YES" ] ; then
    if [ "$SENSITIVITY" == "NONE" ] ; then
        SENSITIVITY="LOW"
    fi
    if [ "$SENSITIVITY" == "LOW" ] ; then
        $SONOFF_HACK_PREFIX/bin/ipc_cmd -s low
    elif [ "$SENSITIVITY" == "MEDIUM" ]; then
        $SONOFF_HACK_PREFIX/bin/ipc_cmd -s medium
    elif [ "$SENSITIVITY" == "HIGH" ]; then
        $SONOFF_HACK_PREFIX/bin/ipc_cmd -s high
    fi
fi

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
