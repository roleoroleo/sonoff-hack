#!/bin/sh

CONF_FILE="etc/system.conf"

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

get_config()
{
    key=$1
    grep -w $1 $SONOFF_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2
}

LOCAL_IP_E=$(ifconfig eth0 | awk '/inet addr/{print substr($2,6)}')
LOCAL_IP_W=$(ifconfig ra0 | awk '/inet addr/{print substr($2,6)}')

if [[ "$LOCAL_IP_E" != "" ]] ; then
    LOCAL_IP=$LOCAL_IP_E
else
    LOCAL_IP=$LOCAL_IP_W
fi

case $(get_config HTTPD_PORT) in
    ''|*[!0-9]*) HTTPD_PORT=8080 ;;
    *) HTTPD_PORT=$(get_config HTTPD_PORT) ;;
esac

if [[ $HTTPD_PORT != "80" ]] ; then
    D_HTTPD_PORT=:$HTTPD_PORT
fi

if [ x$(get_config USERNAME) == "x" ]; then
    RTSP_USERPWD="hack:hack@"
fi

printf "Content-type: application/json\r\n\r\n"
printf "{\n"

printf "\"%s\":\"%s\",\n" "low_res_stream"        "rtsp://$RTSP_USERPWD$LOCAL_IP/av_stream/ch1"
printf "\"%s\":\"%s\",\n" "high_res_stream"       "rtsp://$RTSP_USERPWD$LOCAL_IP/av_stream/ch0"

printf "\"%s\":\"%s\"\n" "high_res_snapshot"      "http://$LOCAL_IP$D_HTTPD_PORT/cgi-bin/snapshot.sh"

printf "}"
