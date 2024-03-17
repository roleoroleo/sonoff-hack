#!/bin/sh

CONF_FILE="etc/system.conf"
MQTT_CONF_FILE="etc/mqtt-sonoff.conf"

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"
SONOFF_HACK_UPGRADE_PATH="/mnt/mmc/.fw_upgrade"

SONOFF_HACK_VER=$(cat /mnt/mmc/sonoff-hack/version)
MODEL_CFG_FILE=/mnt/mtd/ipc/cfg/config_cst.cfg
[ -f /mnt/mtd/db/conf/config_cst.ini ] && MODEL_CFG_FILE=/mnt/mtd/db/conf/config_cst.ini
MODEL=$(cat $MODEL_CFG_FILE | grep model | cut -d'=' -f2 | cut -d'"' -f2)
DEVICE_ID=$(cat /mnt/mtd/ipc/cfg/colink.conf | grep devid | cut -d'=' -f2 | cut -d'"' -f2)

get_config()
{
    key=$1
    grep -w $1 $SONOFF_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2
}

get_mqtt_config()
{
    key=$1
    grep -w $1 $SONOFF_HACK_PREFIX/$MQTT_CONF_FILE | cut -d "=" -f2
}

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/mmc/sonoff-hack/lib
export PATH=$PATH:/mnt/mmc/sonoff-hack/bin:/mnt/mmc/sonoff-hack/sbin:/mnt/mmc/sonoff-hack/usr/bin:/mnt/mmc/sonoff-hack/usr/sbin

# Backup original db
if [ ! -d "/mnt/mmc/db_hack_backup" ]; then
    mkdir -p /mnt/mmc/db_hack_backup
    cp /mnt/mtd/db/* /mnt/mmc/db_hack_backup
fi

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
hostname -F $SONOFF_HACK_PREFIX/etc/hostname
TIMEZONE_N=$(sqlite3 /mnt/mtd/db/ipcsys.db "select c_param_value from t_sys_param where c_param_name='ZoneTimeName';")
if [ ! -z $TIMEZONE_N ]; then
    let TIMEZONE_N=TIMEZONE_N-1
    TIMEZONE=$(sqlite3 /mnt/mtd/db/ipcsys.db "select * from t_zonetime_info LIMIT 1 OFFSET $TIMEZONE_N;")
    export TZ=$(echo $TIMEZONE | cut -d"|" -f1)
fi


if [[ $(get_config SYSLOGD) == "yes" ]] ; then
    syslogd
fi

if [[ $(get_config SWAP_FILE) == "yes" ]] ; then
    SD_PRESENT=$(mount | grep mmc | grep -c ^)
    if [[ $SD_PRESENT -ge 1 ]]; then
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
if [ -f /mnt/mtd/ipc/app/lib/libhardware.so ] && [ ! -f /mnt/mtd/ipc/app/lib/libptz.so ]; then
    PTZ_BIN="/mnt/mmc/sonoff-hack/bin/ptz_h"
else
    PTZ_BIN="/mnt/mmc/sonoff-hack/bin/ptz_p"
fi
diff $PTZ_BIN /mnt/mmc/sonoff-hack/bin/ptz
if [ $? -gt 0 ]; then
    cp $PTZ_BIN /mnt/mmc/sonoff-hack/bin/ptz
fi

#10001|1|admin|12345678|
#10002|1|rtsp|12345678|
# Add hack user if it doesn't exist
HACK_DBUSER=$(sqlite3 /mnt/mtd/db/ipcsys.db "select count(*) from t_user where C_UserID=10101;")
if [[ $HACK_DBUSER -eq 0 ]]; then
    sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
insert into t_user (C_UserID, c_role_id, C_UserName, C_PassWord) values (10101, 1, 'hack', 'hack');
EOF
fi

if [[ $(get_config DISABLE_CLOUD) == "yes" ]] ; then
    # Remove embedded users
    sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
delete from t_user where C_UserId < 10100;
EOF

else
    # Restore admin user if it doesn't exist
    EMB_DBUSER=$(sqlite3 /mnt/mtd/db/ipcsys.db "select count(*) from t_user where C_UserName='admin';")
    # Add hack user if it doesn't exist
    if [[ $EMB_DBUSER -eq 0 ]]; then
        sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
insert into t_user (C_UserID, c_role_id, C_UserName, C_PassWord) values (10001, 1, 'admin', '12345678');
EOF
    fi

    # Restore rtsp user if it doesn't exist
    EMB_DBUSER=$(sqlite3 /mnt/mtd/db/ipcsys.db "select count(*) from t_user where C_UserName='rtsp';")
    # Add hack user if it doesn't exist
    if [[ $EMB_DBUSER -eq 0 ]]; then
        sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
insert into t_user (C_UserID, c_role_id, C_UserName, C_PassWord) values (10002, 1, 'rtsp', '12345678');
EOF
    fi

fi

if [[ x$(get_config USERNAME) != "x" ]] ; then
    USERNAME=$(get_config USERNAME)
    PASSWORD=$(get_config PASSWORD)
    RTSP_USERPWD=""
    ONVIF_USERPWD="user=$USERNAME\npassword=$PASSWORD"
    echo "/onvif::" > /tmp/httpd.conf
    echo "/:$USERNAME:$PASSWORD" >> /tmp/httpd.conf
    chmod 0600 /tmp/httpd.conf
else
    RTSP_USERPWD="hack:hack@"
    echo "/onvif::" > /tmp/httpd.conf
    chmod 0600 /tmp/httpd.conf
fi

PASSWORD_MD5='$1$$qRPK7m23GJusamGpoGLby/'
if [[ x$(get_config SSH_PASSWORD) != "x" ]] ; then
    SSH_PASSWORD=$(get_config SSH_PASSWORD)
    PASSWORD_MD5="$(echo "${SSH_PASSWORD}" | mkpasswd --method=MD5 --stdin)"
fi
sed -i 's|^\(root:\)[^:]*:|root:'${PASSWORD_MD5}':|g' "$SONOFF_HACK_PREFIX/etc/shadow"
mount -o bind $SONOFF_HACK_PREFIX/etc/passwd /etc/passwd
mount -o bind $SONOFF_HACK_PREFIX/etc/shadow /etc/shadow
mount -o bind $SONOFF_HACK_PREFIX/etc/profile /etc/profile

case $(get_config RTSP_PORT) in
    ''|*[!0-9]*) RTSP_PORT=554 ;;
    *) RTSP_PORT=$(get_config RTSP_PORT) ;;
esac
case $(get_config HTTPD_PORT) in
    ''|*[!0-9]*) HTTPD_PORT=80 ;;
    *) HTTPD_PORT=$(get_config HTTPD_PORT) ;;
esac

# Restart ptz driver
PTZ_PRESENT=$(cat /proc/modules | grep ptz_drv | grep -c ^)
if [[ $PTZ_PRESENT -eq 1 ]]; then
    rmmod ptz_drv.ko
    sleep 1
    insmod /mnt/mtd/ipc//app/drive/ptz_drv.ko factory="Links" AutoRun=1 Horizontal=3500 Vertical=900
    if [[ $(get_config PTZ_PRESET_BOOT) != "default" ]] ; then
        (sleep 20 && /mnt/mmc/sonoff-hack/bin/ptz -a go_preset -f $SONOFF_HACK_PREFIX/etc/ptz_presets.conf -n $(get_config PTZ_PRESET_BOOT)) &
    fi

    ROTATE=$(sqlite3 /mnt/mtd/db/ipcsys.db "select c_param_value from t_sys_param where c_param_name=\"mirror\";")
    if [ "$ROTATE" == "0" ]; then
        ROTATE="no"
    else
        ROTATE="yes"
        touch /tmp/.mirror
    fi
fi

if [[ $(get_config DISABLE_CLOUD) == "yes" ]] ; then
    cp /etc/hosts /tmp
    mount -o bind /tmp/hosts /etc/hosts
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
[ $(ps | grep '/mnt/mtd/ipc/app/rtspd' | grep -v grep | grep -c ^) -eq 0 ] && /mnt/mtd/ipc/app/rtspd &

if [[ $(get_config HTTPD) == "yes" ]] ; then
    mkdir -p /mnt/mmc/alarm_record
    mkdir -p /mnt/mmc/sonoff-hack/www/alarm_record
    mount --bind /mnt/mmc/alarm_record /mnt/mmc/sonoff-hack/www/alarm_record
    /mnt/mmc/sonoff-hack/usr/sbin/httpd -p $HTTPD_PORT -h $SONOFF_HACK_PREFIX/www/ -c /tmp/httpd.conf
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
    dropbear -r $SONOFF_HACK_PREFIX/etc/dropbear/dropbear_ecdsa_host_key
fi

if [[ $(get_config NTPD) == "yes" ]] ; then
    # Wait until all the other processes have been initialized
    sleep 5 && ntpd -p $(get_config NTP_SERVER) &
fi

if [[ $(get_config MQTT) == "yes" ]] ; then
    if [[ $(get_mqtt_config HA_DISCOVERY) == "1" ]] ; then
        $SONOFF_HACK_PREFIX/bin/mqtt-sonoff -a &
    else
        $SONOFF_HACK_PREFIX/bin/mqtt-sonoff &
    fi
fi

if [[ $RTSP_PORT != "554" ]] ; then
    D_RTSP_PORT=:$RTSP_PORT
fi

if [[ $HTTPD_PORT != "80" ]] ; then
    D_HTTPD_PORT=:$HTTPD_PORT
fi

if [[ $(get_config SNAPSHOT) == "no" ]] ; then
    touch /tmp/snapshot.disabled
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
    if [[ $(get_config ONVIF_FAULT_IF_UNKNOWN) == "yes" ]] ; then
        ONVIF_FAULT_IF_UNKNOWN=1
    else
        ONVIF_FAULT_IF_UNKNOWN=0
    fi
    if [[ $(get_config ONVIF_SYNOLOGY_NVR) == "yes" ]] ; then
        ONVIF_SYNOLOGY_NVR=1
    else
        ONVIF_SYNOLOGY_NVR=0
    fi

    ONVIF_PROFILE_1="name=Profile_1\nwidth=640\nheight=360\nurl=rtsp://$RTSP_USERPWD%s/av_stream/ch1\nsnapurl=http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh\ntype=H264\ndecoder=NONE"
    ONVIF_PROFILE_0="name=Profile_0\nwidth=1920\nheight=1080\nurl=rtsp://$RTSP_USERPWD%s/av_stream/ch0\nsnapurl=http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh\ntype=H264\ndecoder=NONE"

    ONVIF_SRVD_CONF="/tmp/onvif_simple_server.conf"

    echo "model=Sonoff Hack" > $ONVIF_SRVD_CONF
    echo "manufacturer=Sonoff" >> $ONVIF_SRVD_CONF
    echo "firmware_ver=$SONOFF_HACK_VER" >> $ONVIF_SRVD_CONF
    echo "hardware_id=$MODEL" >> $ONVIF_SRVD_CONF
    echo "serial_num=$DEVICE_ID" >> $ONVIF_SRVD_CONF
    echo "ifs=$ONVIF_NETIF" >> $ONVIF_SRVD_CONF
    echo "port=$HTTPD_PORT" >> $ONVIF_SRVD_CONF
    echo "scope=onvif://www.onvif.org/Profile/Streaming" >> $ONVIF_SRVD_CONF
    echo "adv_fault_if_unknown=$ONVIF_FAULT_IF_UNKNOWN" >> $ONVIF_SRVD_CONF
    echo "adv_synology_nvr=$ONVIF_SYNOLOGY_NVR" >> $ONVIF_SRVD_CONF
    echo "" >> $ONVIF_SRVD_CONF
    if [ ! -z $ONVIF_USERPWD ]; then
        echo -e $ONVIF_USERPWD >> $ONVIF_SRVD_CONF
        echo "" >> $ONVIF_SRVD_CONF
    fi
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

    if [[ $PTZ_PRESENT -eq 1 ]]; then
        echo "#PTZ" >> $ONVIF_SRVD_CONF
        echo "ptz=1" >> $ONVIF_SRVD_CONF
        echo "get_position=/mnt/mmc/sonoff-hack/bin/ptz -a get_coord" >> $ONVIF_SRVD_CONF
        echo "is_moving=echo 0" >> $ONVIF_SRVD_CONF
        if [ ! -f /tmp/.mirror ]; then
            echo "move_left=/mnt/mmc/sonoff-hack/bin/ptz -a left" >> $ONVIF_SRVD_CONF
            echo "move_right=/mnt/mmc/sonoff-hack/bin/ptz -a right" >> $ONVIF_SRVD_CONF
            echo "move_up=/mnt/mmc/sonoff-hack/bin/ptz -a up" >> $ONVIF_SRVD_CONF
            echo "move_down=/mnt/mmc/sonoff-hack/bin/ptz -a down" >> $ONVIF_SRVD_CONF
        else
            echo "move_left=/mnt/mmc/sonoff-hack/bin/ptz -a right" >> $ONVIF_SRVD_CONF
            echo "move_right=/mnt/mmc/sonoff-hack/bin/ptz -a left" >> $ONVIF_SRVD_CONF
            echo "move_up=/mnt/mmc/sonoff-hack/bin/ptz -a down" >> $ONVIF_SRVD_CONF
            echo "move_down=/mnt/mmc/sonoff-hack/bin/ptz -a up" >> $ONVIF_SRVD_CONF
        fi
        echo "move_stop=/mnt/mmc/sonoff-hack/bin/ptz -a stop" >> $ONVIF_SRVD_CONF
        echo "move_preset=/mnt/mmc/sonoff-hack/bin/ptz -f /mnt/mmc/sonoff-hack/etc/ptz_presets.conf -a go_preset -n %d > /dev/null" >> $ONVIF_SRVD_CONF
        echo "set_preset=/mnt/mmc/sonoff-hack/bin/ptz -f /mnt/mmc/sonoff-hack/etc/ptz_presets.conf -a set_preset -e %s" >> $ONVIF_SRVD_CONF
        echo "set_home_position=/mnt/mmc/sonoff-hack/bin/ptz -f /mnt/mmc/sonoff-hack/etc/ptz_presets.conf -a set_home -e Home" >> $ONVIF_SRVD_CONF
        echo "remove_preset=/mnt/mmc/sonoff-hack/bin/ptz -f /mnt/mmc/sonoff-hack/etc/ptz_presets.conf -a del_preset -n %d" >> $ONVIF_SRVD_CONF
        echo "jump_to_abs=/mnt/mmc/sonoff-hack/bin/ptz -a go -X %f -Y %f > /dev/null" >> $ONVIF_SRVD_CONF
        echo "jump_to_rel=/mnt/mmc/sonoff-hack/bin/ptz -a go_rel -X %f -Y %f > /dev/null" >> $ONVIF_SRVD_CONF
        echo "get_presets=/mnt/mmc/sonoff-hack/bin/ptz -f /mnt/mmc/sonoff-hack/etc/ptz_presets.conf -a get_presets" >> $ONVIF_SRVD_CONF
        echo "" >> $ONVIF_SRVD_CONF
    fi

    echo "#EVENT" >> $ONVIF_SRVD_CONF
    echo "events=3" >> $ONVIF_SRVD_CONF
    echo "#Event 0" >> $ONVIF_SRVD_CONF
    echo "topic=tns1:VideoSource/MotionAlarm" >> $ONVIF_SRVD_CONF
    echo "source_name=VideoSourceConfigurationToken" >> $ONVIF_SRVD_CONF
    echo "source_value=VideoSourceToken" >> $ONVIF_SRVD_CONF
    echo "input_file=/tmp/onvif_notify_server/motion_alarm" >> $ONVIF_SRVD_CONF

    chmod 0600 $ONVIF_SRVD_CONF
    onvif_simple_server --conf_file $ONVIF_SRVD_CONF
    mkdir -p /tmp/onvif_notify_server
    onvif_notify_server --conf_file $ONVIF_SRVD_CONF

    if [[ $(get_config ONVIF_WSDD) == "yes" ]] ; then
        wsd_simple_server --pid_file /var/run/wsd_simple_server.pid --if_name $ONVIF_NETIF --xaddr "http://%s$D_HTTPD_PORT/onvif/device_service" -m `hostname` -n Sonoff
    fi
fi

# Run rtsp watchdog
$SONOFF_HACK_PREFIX/script/wd_rtsp.sh &

# Add crontab
CRONTAB=$(get_config CRONTAB)
FREE_SPACE=$(get_config FREE_SPACE)
mkdir -p /var/spool/cron/crontabs/
if [ ! -z "$CRONTAB" ]; then
    echo -e "$CRONTAB" > /var/spool/cron/crontabs/root
fi
if [[ $(get_config SNAPSHOT_VIDEO) == "yes" ]] ; then
    echo "* * * * * /mnt/mmc/sonoff-hack/script/thumb.sh cron" >> /var/spool/cron/crontabs/root
fi
if [ "$FREE_SPACE" != "0" ]; then
    echo "0 * * * * sleep 20; /mnt/mmc/sonoff-hack/script/clean_records.sh $FREE_SPACE" >> /var/spool/cron/crontabs/root
fi
if [[ $(get_config FTP_UPLOAD) == "yes" ]] ; then
    echo "* * * * * sleep 40; /mnt/mmc/sonoff-hack/script/ftppush.sh cron" >> /var/spool/cron/crontabs/root
fi
/usr/sbin/crond -c /var/spool/cron/crontabs/

if [ -f "/mnt/mmc/startup.sh" ]; then
    /mnt/mmc/startup.sh
fi
