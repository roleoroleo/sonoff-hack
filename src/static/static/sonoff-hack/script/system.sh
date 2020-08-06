#!/bin/sh

CONF_FILE="etc/system.conf"

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

SONOFF_HACK_VER=$(cat /mnt/mmc/sonoff-hack/version)
MODEL=$(cat /mnt/mtd/ipc/cfg/config_cst.cfg | grep model | cut -d'=' -f2 | cut -d'"' -f2)

#SERIAL_NUMBER=$(dd status=none bs=1 count=20 skip=661 if=/tmp/mmap.info)
#HW_ID=$(dd status=none bs=1 count=4 skip=661 if=/tmp/mmap.info)
SERIAL_NUMBER="SN"
HW_ID="ID"

get_config()
{
    key=$1
    grep -w $1 $SONOFF_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2
}

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/mmc/sonoff-hack/lib
export PATH=$PATH:/mnt/mmc/sonoff-hack/bin:/mnt/mmc/sonoff-hack/sbin:/mnt/mmc/sonoff-hack/usr/bin:/mnt/mmc/sonoff-hack/usr/sbin

touch /tmp/httpd.conf

# Restore configuration after a firmware upgrade
if [ -f $SONOFF_HACK_PREFIX/.fw_upgrade_in_progress ]; then
    cp -f /tmp/sd/.fw_upgrade/*.conf $SONOFF_HACK_PREFIX/etc/
    chmod 0644 $SONOFF_HACK_PREFIX/etc/*.conf
    if [ -f /tmp/sd/.fw_upgrade/hostname ]; then
        cp -f /tmp/sd/.fw_upgrade/hostname /etc/
        chmod 0644 /etc/hostname
    fi
    if [ -f /tmp/sd/.fw_upgrade/TZ ]; then
        cp -f /tmp/sd/.fw_upgrade/TZ /etc/
        chmod 0644 /etc/TZ
    fi
    rm $SONOFF_HACK_PREFIX/.fw_upgrade_in_progress
    rm -r /tmp/sd/.fw_upgrade
fi

#$SONOFF_HACK_PREFIX/script/check_conf.sh

cp -f $SONOFF_HACK_PREFIX/etc/hostname /etc/hostname
hostname -F /etc/hostname
export TZ=$(get_config TIMEZONE)

if [[ x$(get_config USERNAME) != "x" ]] ; then
    USERNAME=$(get_config USERNAME)
    PASSWORD=$(get_config PASSWORD)
    ONVIF_USERPWD="--user $USERNAME --password $PASSWORD"
    echo "/:$USERNAME:$PASSWORD" > /tmp/httpd.conf
fi

cp -f $SONOFF_HACK_PREFIX/etc/passwd /etc/passwd
cp -f $SONOFF_HACK_PREFIX/etc/shadow /etc/shadow
PASSWORD_MD5='$1$$qRPK7m23GJusamGpoGLby/'
if [[ x$(get_config SSH_PASSWORD) != "x" ]] ; then
    SSH_PASSWORD=$(get_config SSH_PASSWORD)
    PASSWORD_MD5="$(echo "${SSH_PASSWORD}" | mkpasswd --method=MD5 --stdin)"
fi
CUR_PASSWORD_MD5=$(awk -F":" '$1 == "root" { print $2 } ' /etc/shadow)
if [[ x$CUR_PASSWORD_MD5 != x$PASSWORD_MD5 ]] ; then
    sed -i 's|^\(root:\)[^:]*:|root:'${PASSWORD_MD5}':|g' "/etc/shadow"
fi

#case $(get_config RTSP_PORT) in
#    ''|*[!0-9]*) RTSP_PORT=554 ;;
#    *) RTSP_PORT=$(get_config RTSP_PORT) ;;
#esac
case $(get_config ONVIF_PORT) in
    ''|*[!0-9]*) ONVIF_PORT=80 ;;
    *) ONVIF_PORT=$(get_config ONVIF_PORT) ;;
esac
case $(get_config HTTPD_PORT) in
    ''|*[!0-9]*) HTTPD_PORT=8080 ;;
    *) HTTPD_PORT=$(get_config HTTPD_PORT) ;;
esac

if [[ $(get_config HTTPD) == "yes" ]] ; then
    httpd -p $HTTPD_PORT -h $SONOFF_HACK_PREFIX/www/ -c /tmp/httpd.conf
fi

if [[ $(get_config TELNETD) == "no" ]] ; then
    killall telnetd
fi

if [[ $(get_config FTPD) == "yes" ]] ; then
    if [[ $(get_config BUSYBOX_FTPD) == "yes" ]] ; then
        tcpsvd -vE 0.0.0.0 21 ftpd -w &
    else
        pure-ftpd -B
    fi
fi

if [[ $(get_config SSHD) == "yes" ]] ; then
    # Restore keys
    mkdir -p /etc/dropbear
    cp -f $SONOFF_HACK_PREFIX/etc/dropbear/* /etc/dropbear/
    chmod 0600 /etc/dropbear/*
    dropbear -R
    # Backup keys
    mkdir -p $SONOFF_HACK_PREFIX/etc/dropbear
    cp -f /etc/dropbear/* $SONOFF_HACK_PREFIX/etc/dropbear
fi

if [[ $(get_config NTPD) == "yes" ]] ; then
    # Wait until all the other processes have been initialized
    sleep 5 && ntpd -p $(get_config NTP_SERVER) &
fi

#if [[ $(get_config MQTT) == "yes" ]] ; then
#    mqttv4 &
#fi
#
#sleep 5

if [[ $RTSP_PORT != "554" ]] ; then
    D_RTSP_PORT=:$RTSP_PORT
fi

if [[ $HTTPD_PORT != "80" ]] ; then
    D_HTTPD_PORT=:$HTTPD_PORT
fi

if [[ $ONVIF_PORT != "80" ]] ; then
    D_ONVIF_PORT=:$ONVIF_PORT
fi

#if [[ $(get_config ONVIF_WM_SNAPSHOT) == "yes" ]] ; then
#    WATERMARK="&watermark=yes"
#fi

if [[ $(get_config ONVIF_NETIF) == "ra0" ]] ; then
    ONVIF_NETIF="ra0"
else
    ONVIF_NETIF="eth0"
fi

ONVIF_PROFILE_1="--name Profile_1 --width 640 --height 360 --url rtsp://%s$D_RTSP_PORT/ch0_1.h264 --snapurl http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh?res=low --type H264"
ONVIF_PROFILE_0="--name Profile_0 --width 1920 --height 1080 --url rtsp://%s$D_RTSP_PORT/ch0_0.h264 --snapurl http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh?res=high --type H264"

if [[ $(get_config ONVIF) == "yes" ]] ; then
    onvif_srvd --pid_file /var/run/onvif_srvd.pid --model "Sonoff Hack" --manufacturer "Sonoff" --firmware_ver "$SONOFF_HACK_VER" --hardware_id $HW_ID --serial_num $SERIAL_NUMBER --ifs $ONVIF_NETIF --port $ONVIF_PORT --scope onvif://www.onvif.org/Profile/S $ONVIF_PROFILE_0 $ONVIF_PROFILE_1 $ONVIF_USERPWD --ptz --move_left "/mnt/mmc/sonoff-hack/bin/ptz -a left" --move_right "/mnt/mmc/sonoff-hack/bin/ptz -a right" --move_up "/mnt/mmc/sonoff-hack/bin/ptz -a up" --move_down "/mnt/mmc/sonoff-hack/bin/ptz -a down" --move_stop "/mnt/mmc/sonoff-hack/bin/ptz -a stop"
# --move_preset "/mnt/mmc/sonoff-hack/bin/ptz -p"
    if [[ $(get_config ONVIF_WSDD) == "yes" ]] ; then
        wsdd --pid_file /var/run/wsdd.pid --if_name $ONVIF_NETIF --type tdn:NetworkVideoTransmitter --xaddr http://%s$D_ONVIF_PORT --scope "onvif://www.onvif.org/name/Unknown onvif://www.onvif.org/Profile/Streaming"
    fi
fi

#FREE_SPACE=$(get_config FREE_SPACE)
#if [[ $FREE_SPACE != "0" ]] ; then
#    mkdir -p /var/spool/cron/crontabs/
#    echo "  0  *  *  *  *  /mnt/mmc/sonoff-hack/script/clean_records.sh $FREE_SPACE" > /var/spool/cron/crontabs/root
#    /usr/sbin/crond -c /var/spool/cron/crontabs/
#fi
#
#if [[ $(get_config FTP_UPLOAD) == "yes" ]] ; then
#    /mnt/mmc/sonoff-hack/script/ftppush.sh start &
#fi
