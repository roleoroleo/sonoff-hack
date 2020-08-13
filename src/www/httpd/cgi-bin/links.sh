#!/bin/sh

LOCAL_IP_E=$(ifconfig eth0 | awk '/inet addr/{print substr($2,6)}')
LOCAL_IP_W=$(ifconfig ra0 | awk '/inet addr/{print substr($2,6)}')

if [[ "$LOCAL_IP_E" != "" ]] ; then
    LOCAL_IP=$LOCAL_IP_E
else
    LOCAL_IP=$LOCAL_IP_W
fi

#case $(get_config RTSP_PORT) in
#    ''|*[!0-9]*) RTSP_PORT=554 ;;
#    *) RTSP_PORT=$(get_config RTSP_PORT) ;;
#esac
RTSP_PORT=554
case $(get_config HTTPD_PORT) in
    ''|*[!0-9]*) HTTPD_PORT=8080 ;;
    *) HTTPD_PORT=$(get_config HTTPD_PORT) ;;
esac

if [[ $RTSP_PORT != "554" ]] ; then
    D_RTSP_PORT=:$RTSP_PORT
fi
if [[ $HTTPD_PORT != "80" ]] ; then
    D_HTTPD_PORT=:$HTTPD_PORT
fi

RTSP_USER="rtsp"
RTSP_PWD=$(/mnt/mmc/sonoff-hack/bin/sqlite3 /mnt/mtd/db/ipcsys.db "select c_PassWord from t_user where c_UserName='$RTSP_USER';")

printf "Content-type: application/json\r\n\r\n"
printf "{\n"

printf "\"%s\":\"%s\",\n" "low_res_stream"        "rtsp://$RTSP_USER:$RTSP_PWD@$LOCAL_IP$D_RTSP_PORT/av_stream/ch1"
printf "\"%s\":\"%s\",\n" "high_res_stream"       "rtsp://$RTSP_USER:$RTSP_PWD@$LOCAL_IP$D_RTSP_PORT/av_stream/ch0"

printf "\"%s\":\"%s\"\n" "high_res_snapshot"      "http://$LOCAL_IP$D_HTTPD_PORT/cgi-bin/snapshot.sh"

printf "}"
