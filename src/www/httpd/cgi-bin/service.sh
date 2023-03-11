#!/bin/sh

CONF_FILE="etc/system.conf"

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

SONOFF_HACK_VER=$(cat /mnt/mmc/sonoff-hack/version)
MODEL=$(cat /mnt/mtd/ipc/cfg/config_cst.cfg | grep model | cut -d'=' -f2 | cut -d'"' -f2)
DEVICE_ID=$(cat /mnt/mtd/ipc/cfg/colink.conf | grep devid | cut -d'=' -f2 | cut -d'"' -f2)

get_config()
{
    key=$1
    grep -w $1 $SONOFF_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2
}

init_config()
{
    if [[ x$(get_config USERNAME) != "x" ]] ; then
        USERNAME=$(get_config USERNAME)
        PASSWORD=$(get_config PASSWORD)
        RTSP_USERPWD=""
        ONVIF_USERPWD="user=$USERNAME\npassword=$PASSWORD"
        echo "/:$USERNAME:$PASSWORD" > /tmp/httpd.conf
    else
        RTSP_USERPWD="hack:hack@"
    fi

    case $(get_config RTSP_PORT) in
        ''|*[!0-9]*) RTSP_PORT=554 ;;
        *) RTSP_PORT=$(get_config RTSP_PORT) ;;
    esac
    case $(get_config ONVIF_PORT) in
        ''|*[!0-9]*) ONVIF_PORT=1000 ;;
        *) ONVIF_PORT=$(get_config ONVIF_PORT) ;;
    esac
    case $(get_config HTTPD_PORT) in
        ''|*[!0-9]*) HTTPD_PORT=80 ;;
        *) HTTPD_PORT=$(get_config HTTPD_PORT) ;;
    esac

    if [[ $RTSP_PORT != "554" ]] ; then
        D_RTSP_PORT=:$RTSP_PORT
    fi

    if [[ $ONVIF_PORT != "80" ]] ; then
        D_ONVIF_PORT=:$ONVIF_PORT
    fi

    if [[ $HTTPD_PORT != "80" ]] ; then
        D_HTTPD_PORT=:$HTTPD_PORT
    fi

    if [[ $(get_config ONVIF_NETIF) == "ra0" ]] ; then
        ONVIF_NETIF="ra0"
    else
        ONVIF_NETIF="eth0"
    fi
}

start_rtsp()
{
    /mnt/mtd/ipc/app/rtspd >/dev/null &
}

stop_rtsp()
{
    killall rtspd
}

start_onvif()
{
    if [[ "$1" == "none" ]]; then
        ONVIF_PROFILE=$(get_config ONVIF_PROFILE)
    elif [[ "$1" == "low" ]] || [[ "$1" == "high" ]] || [[ "$1" == "both" ]]; then
        ONVIF_PROFILE=$1
    fi
    if [[ $ONVIF_PROFILE == "high" ]]; then
        ONVIF_PROFILE_0="name=Profile_0\nwidth=1920\nheight=1080\nurl=rtsp://$RTSP_USERPWD%s/av_stream/ch0\nsnapurl=http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh\ntype=H264"
    fi
    if [[ $ONVIF_PROFILE == "low" ]]; then
        ONVIF_PROFILE_1="name=Profile_1\nwidth=640\nheight=360\nurl=rtsp://$RTSP_USERPWD%s/av_stream/ch1\nsnapurl=http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh\ntype=H264"
    fi
    if [[ $ONVIF_PROFILE == "both" ]]; then
        ONVIF_PROFILE_0="name=Profile_0\nwidth=1920\nheight=1080\nurl=rtsp://$RTSP_USERPWD%s/av_stream/ch0\nsnapurl=http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh\ntype=H264"
        ONVIF_PROFILE_1="name=Profile_1\nwidth=640\nheight=360\nurl=rtsp://$RTSP_USERPWD%s/av_stream/ch1\nsnapurl=http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh\ntype=H264"
    fi

    ONVIF_SRVD_CONF="/tmp/onvif_srvd.conf"

    echo "pid_file=/var/run/onvif_srvd.pid" > $ONVIF_SRVD_CONF
    echo "model=Sonoff Hack" >> $ONVIF_SRVD_CONF
    echo "manufacturer=Sonoff" >> $ONVIF_SRVD_CONF
    echo "firmware_ver=$SONOFF_HACK_VER" >> $ONVIF_SRVD_CONF
    echo "hardware_id=$MODEL" >> $ONVIF_SRVD_CONF
    echo "serial_num=$DEVICE_ID" >> $ONVIF_SRVD_CONF
    echo "ifs=$ONVIF_NETIF" >> $ONVIF_SRVD_CONF
    echo "port=$ONVIF_PORT" >> $ONVIF_SRVD_CONF
    echo "scope=onvif://www.onvif.org/Profile/Streaming" >> $ONVIF_SRVD_CONF
    echo "" >> $ONVIF_SRVD_CONF
    if [ ! -z $ONVIF_PROFILE_0 ]; then
        echo "#Profile 0" >> $ONVIF_SRVD_CONF
        echo -e $ONVIF_PROFILE_0 >> $ONVIF_SRVD_CONF
        echo "" >> $ONVIF_SRVD_CONF
    fi
    if [ ! -z $ONVIF_PROFILE_1 ]; then
        echo "#Profile 1" >> $ONVIF_SRVD_CONF
        echo -e $ONVIF_PROFILE_1 >> $ONVIF_SRVD_CONF
        echo "" >> $ONVIF_SRVD_CONF
    fi
    if [ ! -z $ONVIF_USERPWD ]; then
        echo -e $ONVIF_USERPWD >> $ONVIF_SRVD_CONF
        echo "" >> $ONVIF_SRVD_CONF
    fi

    echo "#PTZ" >> $ONVIF_SRVD_CONF
    echo "ptz=1" >> $ONVIF_SRVD_CONF
    if [ -f /tmp/.mirror ]; then
        echo "move_left=/mnt/mmc/sonoff-hack/bin/ptz -a right" >> $ONVIF_SRVD_CONF
        echo "move_right=/mnt/mmc/sonoff-hack/bin/ptz -a left" >> $ONVIF_SRVD_CONF
        echo "move_up=/mnt/mmc/sonoff-hack/bin/ptz -a down" >> $ONVIF_SRVD_CONF
        echo "move_down=/mnt/mmc/sonoff-hack/bin/ptz -a up" >> $ONVIF_SRVD_CONF
    else
        echo "move_left=/mnt/mmc/sonoff-hack/bin/ptz -a left" >> $ONVIF_SRVD_CONF
        echo "move_right=/mnt/mmc/sonoff-hack/bin/ptz -a right" >> $ONVIF_SRVD_CONF
        echo "move_up=/mnt/mmc/sonoff-hack/bin/ptz -a up" >> $ONVIF_SRVD_CONF
        echo "move_down=/mnt/mmc/sonoff-hack/bin/ptz -a down" >> $ONVIF_SRVD_CONF
    fi
    echo "move_stop=/mnt/mmc/sonoff-hack/bin/ptz -a stop" >> $ONVIF_SRVD_CONF
    echo "move_preset=/mnt/mmc/sonoff-hack/bin/ptz -f /mnt/mmc/sonoff-hack/etc/ptz_presets.conf -a go_preset -n %t" >> $ONVIF_SRVD_CONF
    echo "set_preset=/mnt/mmc/sonoff-hack/bin/ptz -f /mnt/mmc/sonoff-hack/etc/ptz_presets.conf -a set_preset -e %n -n %t" >> $ONVIF_SRVD_CONF

    onvif_srvd --conf_file $ONVIF_SRVD_CONF
}

stop_onvif()
{
    killall onvif_srvd
}

start_wsdd()
{
    wsdd --pid_file /var/run/wsdd.pid --if_name $ONVIF_NETIF --type tdn:NetworkVideoTransmitter --xaddr http://%s$D_ONVIF_PORT --scope "onvif://www.onvif.org/name/Unknown onvif://www.onvif.org/Profile/Streaming"
}

stop_wsdd()
{
    killall wsdd
}

start_ftpd()
{
    if [[ "$1" == "none" ]] ; then
        if [[ $(get_config BUSYBOX_FTPD) == "yes" ]] ; then
            FTPD_DAEMON="busybox"
        else
            FTPD_DAEMON="pure-ftpd"
        fi
    else
        FTPD_DAEMON=$1
    fi

    if [[ $FTPD_DAEMON == "busybox" ]] ; then
        tcpsvd -vE 0.0.0.0 21 ftpd -w >/dev/null &
    elif [[ $FTPD_DAEMON == "pure-ftpd" ]] ; then
        pure-ftpd -B
    fi
}

stop_ftpd()
{
    if [[ "$1" == "none" ]] ; then
        if [[ $(get_config BUSYBOX_FTPD) == "yes" ]] ; then
            FTPD_DAEMON="busybox"
        else
            FTPD_DAEMON="pure-ftpd"
        fi
    else
        FTPD_DAEMON=$1
    fi

    if [[ $FTPD_DAEMON == "busybox" ]] ; then
        killall tcpsvd
    elif [[ $FTPD_DAEMON == "pure-ftpd" ]] ; then
        killall pure-ftpd
    fi
}

ps_program()
{
    PS_PROGRAM=$(ps | grep $1 | grep -v grep | grep -c ^)
    if [ $PS_PROGRAM -gt 0 ]; then
        echo "started"
    else
        echo "stopped"
    fi
}

. $SONOFF_HACK_PREFIX/www/cgi-bin/validate.sh

if ! $(validateQueryString $QUERY_STRING); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

NAME="none"
ACTION="none"
PARAM1="none"
RES=""

for I in 1 2 3
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "name" ] ; then
        NAME="$VAL"
    elif [ "$CONF" == "action" ] ; then
        ACTION="$VAL"
    elif [ "$CONF" == "param1" ] ; then
        PARAM1="$VAL"
    fi
done

init_config

if [ "$ACTION" == "start" ] ; then
    if [ "$NAME" == "rtsp" ]; then
        start_rtsp
    elif [ "$NAME" == "onvif" ]; then
        start_onvif $PARAM1
    elif [ "$NAME" == "wsdd" ]; then
        start_wsdd
    elif [ "$NAME" == "ftpd" ]; then
        start_ftpd $PARAM1
    elif [ "$NAME" == "mqtt" ]; then
        mqtt-sonoff > /dev/null &
    elif [ "$NAME" == "all" ]; then
        start_rtsp
        start_onvif
        start_wsdd
        start_ftpd
        mqtt-sonoff > /dev/null &
    fi
elif [ "$ACTION" == "stop" ] ; then
    if [ "$NAME" == "rtsp" ]; then
        stop_rtsp
    elif [ "$NAME" == "onvif" ]; then
        stop_onvif
    elif [ "$NAME" == "wsdd" ]; then
        stop_wsdd
    elif [ "$NAME" == "ftpd" ]; then
        stop_ftpd $PARAM1
    elif [ "$NAME" == "mqtt" ]; then
        killall mqtt-sonoff
    elif [ "$NAME" == "all" ]; then
        stop_rtsp
        stop_onvif
        stop_wsdd
        stop_ftpd
        killall mqtt-sonoff
    fi
elif [ "$ACTION" == "status" ] ; then
    if [ "$NAME" == "rtsp" ]; then
        RES=$(ps_program rtspd)
    elif [ "$NAME" == "onvif" ]; then
        RES=$(ps_program onvif_srvd)
    elif [ "$NAME" == "wsdd" ]; then
        RES=$(ps_program wsdd)
    elif [ "$NAME" == "ftpd" ]; then
        RES=$(ps_program ftpd)
    elif [ "$NAME" == "mqtt" ]; then
        RES=$(ps_program mqtt-sonoff)
    fi
fi

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
if [ ! -z "$RES" ]; then
    printf "\"status\": \"$RES\"\n"
fi
printf "}"
