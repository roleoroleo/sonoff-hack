#!/bin/sh

CONF_FILE="etc/system.conf"

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"
SONOFF_HACK_UPGRADE_PATH="/mnt/mmc/.fw_upgrade"

SONOFF_HACK_VER=$(cat /mnt/mmc/sonoff-hack/version)
MODEL=$(cat /mnt/mtd/ipc/cfg/config_cst.cfg | grep model | cut -d'=' -f2 | cut -d'"' -f2)
DEVICE_ID=$(cat /mnt/mtd/ipc/cfg/colink.conf | grep devid | cut -d'=' -f2 | cut -d'"' -f2)

get_config()
{
    key=$1
    grep -w $1 $SONOFF_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2
}

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/mmc/sonoff-hack/lib
export PATH=$PATH:/mnt/mmc/sonoff-hack/bin:/mnt/mmc/sonoff-hack/sbin:/mnt/mmc/sonoff-hack/usr/bin:/mnt/mmc/sonoff-hack/usr/sbin

touch /tmp/httpd.conf

if [ -f $SONOFF_HACK_UPGRADE_PATH/sonoff-hack/fw_upgrade_in_progress ]; then
    echo "#!/bin/sh" > /tmp/fw_upgrade_2p.sh
    echo "# Complete fw upgrade and restore configuration" >> /tmp/fw_upgrade_2p.sh
    echo "sleep 1" >> /tmp/fw_upgrade_2p.sh
    echo "cd $SONOFF_HACK_UPGRADE_PATH" >> /tmp/fw_upgrade_2p.sh
    echo "cp -rf * .." >> /tmp/fw_upgrade_2p.sh
    echo "cd .." >> /tmp/fw_upgrade_2p.sh
    echo "rm -rf $SONOFF_HACK_UPGRADE_PATH" >> /tmp/fw_upgrade_2p.sh
    echo "rm $SONOFF_HACK_PREFIX/fw_upgrade_in_progress" >> /tmp/fw_upgrade_2p.sh
    echo "sync" >> /tmp/fw_upgrade_2p.sh
    echo "sync" >> /tmp/fw_upgrade_2p.sh
    echo "sync" >> /tmp/fw_upgrade_2p.sh
    echo "reboot" >> /tmp/fw_upgrade_2p.sh
    sh /tmp/fw_upgrade_2p.sh
    exit
fi

$SONOFF_HACK_PREFIX/script/check_conf.sh

cp -f $SONOFF_HACK_PREFIX/etc/hostname /etc/hostname
hostname -F /etc/hostname
export TZ=$(get_config TIMEZONE)

if [[ $(get_config SWAP_FILE) == "yes" ]] ; then
    SD_PRESENT=$(mount | grep mmc | grep -c ^)
    if [[ $SD_PRESENT -eq 1 ]]; then
        if [[ -f /mnt/mmc/swapfile ]]; then
            swapon /mnt/mmc/swapfile
        else
            dd if=/dev/zero of=/mnt/mmc/swapfile bs=1M count=64
            chmod 0600 /mnt/mmc/swapfile
            mkswap /mnt/mmc/swapfile
            swapon /mnt/mmc/swapfile
        fi
    fi
fi

# Check libptz.so or libhardware.so
if [ -f /mnt/mtd/ipc/app/lib/libptz.so ]; then
    if [ ! -f /mnt/mmc/sonoff-hack/bin/ptz ]; then
        cp /mnt/mmc/sonoff-hack/bin/ptz_p /mnt/mmc/sonoff-hack/bin/ptz
    fi
else
    if [ ! -f /mnt/mmc/sonoff-hack/bin/ptz ]; then
        cp /mnt/mmc/sonoff-hack/bin/ptz_h /mnt/mmc/sonoff-hack/bin/ptz
    fi
fi

# Create hack user if doesn't exist
HACK_USER=$(sqlite3 /mnt/mtd/db/ipcsys.db "select count(*) from t_user where C_UserID=10101;")
if [[ $HACK_USER -eq 0 ]]; then
    sqlite3 /mnt/mtd/db/ipcsys.db "insert into t_user (C_UserID, c_role_id, C_UserName, C_PassWord) values (10101, 1, 'hack', 'hack');"
fi

if [[ x$(get_config USERNAME) != "x" ]] ; then
    USERNAME=$(get_config USERNAME)
    PASSWORD=$(get_config PASSWORD)
    RTSP_USERPWD=""
    ONVIF_USERPWD="--user $USERNAME --password $PASSWORD"
    echo "/:$USERNAME:$PASSWORD" > /tmp/httpd.conf
else
    RTSP_USERPWD="hack:hack@"
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

# Restart ptz driver
rmmod ptz_drv.ko
sleep 1
insmod /mnt/mtd/ipc//app/drive/ptz_drv.ko factory="Links" AutoRun=1 Horizontal=3500 Vertical=900
if [[ $(get_config PTZ_PRESET_BOOT) != "default" ]] ; then
    (sleep 20 && /mnt/mmc/sonoff-hack/bin/ptz -a go_preset -f $SONOFF_HACK_PREFIX/etc/ptz_presets.conf -n $(get_config PTZ_PRESET_BOOT)) &
fi

if [[ $(get_config DISABLE_CLOUD) == "yes" ]] ; then
    # Add forbidden domains
    echo "127.0.0.1               eu-dispd.coolkit.cc" >> /etc/hosts
    echo "127.0.0.1               eu-api.coolkit.cn" >> /etc/hosts
    echo "127.0.0.1               testapi.coolkit.cn" >> /etc/hosts
    echo "127.0.0.1               push.iotcare.cn" >> /etc/hosts
    echo "127.0.0.1               www.iotcare.cn" >> /etc/hosts
    echo "127.0.0.1               alive.hapsee.cn" >> /etc/hosts
    echo "127.0.0.1               upgrade.hapsee.cn" >> /etc/hosts
    echo "127.0.0.1               hapseemate.cn" >> /etc/hosts
    echo "127.0.0.1               iotgo.iteadstudio.com" >> /etc/hosts
    echo "127.0.0.1               baidu.com" >> /etc/hosts
    echo "127.0.0.1               sina.com" >> /etc/hosts

    # Add forbidden IPs
    ip route add prohibit 13.52.12.176/32

    # Kill ProcessGuard and iot processes
    touch /tmp/bProcessGuardExit
    killall colinkwtg.sh
    killall colink.sh
    killall colink
    killall IOTCare
else
    umount /mnt/mtd/ipc/app/colink
    rm /tmp/colink
fi

if [[ $(get_config HTTPD) == "yes" ]] ; then
    mkdir -p /mnt/mmc/alarm_record
    mkdir -p /mnt/mmc/sonoff-hack/www/alarm_record
    mount --bind /mnt/mmc/alarm_record /mnt/mmc/sonoff-hack/www/alarm_record
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
    mkdir -p $SONOFF_HACK_PREFIX/etc/dropbear
    if [ ! -f $SONOFF_HACK_PREFIX/etc/dropbear/dropbear_ecdsa_host_key ]; then
        dropbearkey -t ecdsa -f /tmp/dropbear_ecdsa_host_key
        mv /tmp/dropbear_ecdsa_host_key $SONOFF_HACK_PREFIX/etc/dropbear/
    fi
    # Restore keys
    mkdir -p /etc/dropbear
    cp -f $SONOFF_HACK_PREFIX/etc/dropbear/* /etc/dropbear/
    chmod 0600 /etc/dropbear/*
    dropbear -R
fi

if [[ $(get_config NTPD) == "yes" ]] ; then
    # Wait until all the other processes have been initialized
    sleep 5 && ntpd -p $(get_config NTP_SERVER) &
fi

if [[ $(get_config MQTT) == "yes" ]] ; then
    $SONOFF_HACK_PREFIX/bin/mqtt-sonoff &
fi

if [[ $RTSP_PORT != "554" ]] ; then
    D_RTSP_PORT=:$RTSP_PORT
fi

if [[ $ONVIF_PORT != "80" ]] ; then
    D_ONVIF_PORT=:$ONVIF_PORT
fi

if [[ $HTTPD_PORT != "80" ]] ; then
    D_HTTPD_PORT=:$HTTPD_PORT
fi

if [[ "$RTSP_PORT" != "554" ]] ; then
    killall rtspd
    mkdir -p /mnt/mmc/var/av/xml
    cp -r /mnt/mtd/ipc/app/av.xml /mnt/mmc/var/av/xml
    sed -i "s/<VALUE>554<\/VALUE>/<VALUE>$RTSP_PORT<\/VALUE>/g" /mnt/mmc/var/av/xml/av.xml
    mount -o bind,rw /mnt/mmc/var/av/xml/av.xml /mnt/mtd/ipc/app/av.xml
    /mnt/mtd/ipc/app/rtspd >/dev/null &
fi

if [[ $(get_config ONVIF) == "yes" ]] ; then
    if [[ $(get_config ONVIF_NETIF) == "ra0" ]] ; then
        ONVIF_NETIF="ra0"
    else
        ONVIF_NETIF="eth0"
    fi

    ONVIF_PROFILE_1="--name Profile_1 --width 640 --height 360 --url rtsp://$RTSP_USERPWD%s/av_stream/ch1 --snapurl http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh --type H264"
    ONVIF_PROFILE_0="--name Profile_0 --width 1920 --height 1080 --url rtsp://$RTSP_USERPWD%s/av_stream/ch0 --snapurl http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh --type H264"

    onvif_srvd --pid_file /var/run/onvif_srvd.pid --model "Sonoff Hack" --manufacturer "Sonoff" --firmware_ver "$SONOFF_HACK_VER" --hardware_id $MODEL --serial_num $DEVICE_ID --ifs $ONVIF_NETIF --port $ONVIF_PORT --scope onvif://www.onvif.org/Profile/S $ONVIF_PROFILE_0 $ONVIF_PROFILE_1 $ONVIF_USERPWD --ptz --move_left "/mnt/mmc/sonoff-hack/bin/ptz -a left" --move_right "/mnt/mmc/sonoff-hack/bin/ptz -a right" --move_up "/mnt/mmc/sonoff-hack/bin/ptz -a up" --move_down "/mnt/mmc/sonoff-hack/bin/ptz -a down" --move_stop "/mnt/mmc/sonoff-hack/bin/ptz -a stop" --move_preset "/mnt/mmc/sonoff-hack/bin/ptz -f /mnt/mmc/sonoff-hack/etc/ptz_presets.conf -a go_preset -n %t" --set_preset "/mnt/mmc/sonoff-hack/bin/ptz -f /mnt/mmc/sonoff-hack/etc/ptz_presets.conf -a set_preset -e %n -n %t"
    if [[ $(get_config ONVIF_WSDD) == "yes" ]] ; then
        wsdd --pid_file /var/run/wsdd.pid --if_name $ONVIF_NETIF --type tdn:NetworkVideoTransmitter --xaddr http://%s$D_ONVIF_PORT --scope "onvif://www.onvif.org/name/Unknown onvif://www.onvif.org/Profile/Streaming"
    fi
fi

# Add crontab
CRONTAB=$(get_config CRONTAB)
FREE_SPACE=$(get_config FREE_SPACE)
if [ ! -z "$CRONTAB" ] || [ "$FREE_SPACE" != "0" ] ; then
    mkdir -p /var/spool/cron/crontabs/

    if [ ! -z "$CRONTAB" ]; then
        echo "$CRONTAB" > /var/spool/cron/crontabs/root
    fi
    if [ "$FREE_SPACE" != "0" ]; then
        echo "0 * * * * /mnt/mmc/sonoff-hack/script/clean_records.sh $FREE_SPACE" > /var/spool/cron/crontabs/root
    fi

    /usr/sbin/crond -c /var/spool/cron/crontabs/
fi

if [[ $(get_config FTP_UPLOAD) == "yes" ]] ; then
    /mnt/mmc/sonoff-hack/script/ftppush.sh start &
fi

if [ -f "/mnt/mmc/startup.sh" ]; then
    /mnt/mmc/startup.sh
fi
