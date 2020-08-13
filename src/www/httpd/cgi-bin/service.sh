#!/bin/sh

CONF_FILE="etc/system.conf"

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

MODEL=$(cat /mnt/mtd/ipc/cfg/config_cst.cfg | grep model | cut -d'=' -f2 | cut -d'"' -f2)

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
        ONVIF_USERPWD="--user $USERNAME --password $PASSWORD"
    fi

    case $(get_config RTSP_PORT) in
        ''|*[!0-9]*) RTSP_PORT=554 ;;
        *) RTSP_PORT=$(get_config RTSP_PORT) ;;
    esac
    case $(get_config ONVIF_PORT) in
        ''|*[!0-9]*) ONVIF_PORT=80 ;;
        *) ONVIF_PORT=$(get_config ONVIF_PORT) ;;
    esac
    case $(get_config HTTPD_PORT) in
        ''|*[!0-9]*) HTTPD_PORT=8080 ;;
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

start_onvif()
{
    if [[ $1 == "high" ]]; then
        ONVIF_PROFILE_0="--name Profile_0 --width 1920 --height 1080 --url rtsp://%s$D_RTSP_PORT/ch0_0.h264 --snapurl http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh --type H264"
    fi
    if [[ $1 == "low" ]]; then
        ONVIF_PROFILE_1="--name Profile_1 --width 640 --height 360 --url rtsp://%s$D_RTSP_PORT/ch0_1.h264 --snapurl http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh --type H264"
    fi
    if [[ $1 == "both" ]]; then
        ONVIF_PROFILE_0="--name Profile_0 --width 1920 --height 1080 --url rtsp://%s$D_RTSP_PORT/ch0_0.h264 --snapurl http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh --type H264"
        ONVIF_PROFILE_1="--name Profile_1 --width 640 --height 360 --url rtsp://%s$D_RTSP_PORT/ch0_1.h264 --snapurl http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh --type H264"
    fi

    onvif_srvd --pid_file /var/run/onvif_srvd.pid --model "Sonoff Hack" --manufacturer "Sonoff" --firmware_ver "$SONOFF_HACK_VER" --hardware_id $HW_ID --serial_num $SERIAL_NUMBER --ifs $ONVIF_NETIF --port $ONVIF_PORT --scope onvif://www.onvif.org/Profile/S $ONVIF_PROFILE_0 $ONVIF_PROFILE_1 $ONVIF_USERPWD --ptz --move_left "/home/yi-hack/bin/ptz -a left" --move_right "/home/yi-hack/bin/ptz -a right" --move_up "/home/yi-hack/bin/ptz -a up" --move_down "/home/yi-hack/bin/ptz -a down" --move_stop "/home/yi-hack/bin/ptz -a stop"
# --move_preset "/home/yi-hack/bin/ipc_cmd -p"
    wsdd --pid_file /var/run/wsdd.pid --if_name $ONVIF_NETIF --type tdn:NetworkVideoTransmitter --xaddr http://%s$D_ONVIF_PORT --scope "onvif://www.onvif.org/name/Unknown onvif://www.onvif.org/Profile/Streaming"
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
    if [[ $1 == "busybox" ]] ; then
        tcpsvd -vE 0.0.0.0 21 ftpd -w >/dev/null &
    elif [[ $1 == "pure-ftpd" ]] ; then
        pure-ftpd -B
    fi
}

stop_ftpd()
{
    if [[ $1 == "busybox" ]] ; then
        killall tcpsvd
    elif [[ $1 == "pure-ftpd" ]] ; then
        killall pure-ftpd
    fi
}

NAME="none"
ACTION="none"
PARAM1="none"
PARAM2="none"

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
    if [ "$NAME" == "onvif" ]; then
        start_onvif $PARAM1
    elif [ "$NAME" == "wsdd" ]; then
        start_wsdd
	elif [ "$NAME" == "ftpd" ]; then
        start_ftpd $PARAM1
    elif [ "$NAME" == "mqtt" ]; then
        mqttv4 >/dev/null &
    fi
elif [ "$ACTION" == "stop" ] ; then
    if [ "$NAME" == "onvif" ]; then
        stop_onvif $PARAM1
    elif [ "$NAME" == "wsdd" ]; then
        stop_wsdd
    elif [ "$NAME" == "ftpd" ]; then
        stop_ftpd $PARAM1
    elif [ "$NAME" == "mqtt" ]; then
        killall mqttv4
    fi
fi

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "}"
