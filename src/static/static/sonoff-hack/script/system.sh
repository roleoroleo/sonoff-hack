#!/bin/sh

CONF_FILE="etc/system.conf"
MQTT_CONF_FILE="etc/mqtt-sonoff.conf"

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"
SONOFF_HACK_UPGRADE_PATH="/mnt/mmc/.fw_upgrade"

START_STOP_SCRIPT=$SONOFF_HACK_PREFIX/script/service.sh

SONOFF_HACK_VER=$(cat /mnt/mmc/sonoff-hack/version)
MODEL_CFG_FILE=/mnt/mtd/ipc/cfg/config_cst.cfg
[ -f /mnt/mtd/db/conf/config_cst.ini ] && MODEL_CFG_FILE=/mnt/mtd/db/conf/config_cst.ini
MODEL=$(cat $MODEL_CFG_FILE | grep model | cut -d'=' -f2 | cut -d'"' -f2)
DEVICE_ID=$(cat /mnt/mtd/ipc/cfg/colink.conf | grep devid | cut -d'=' -f2 | cut -d'"' -f2)
if [ -z $DEVICE_ID ]; then
    DEVICE_ID= $(cat /mnt/mmc/sonoff-hack/etc/hostname)
fi

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
touch /mnt/mtd/ipc/cfg/guideRstpd

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
    $START_STOP_SCRIPT ftpd start
fi

if [[ $(get_config SSHD) == "yes" ]] ; then
    # show sonoff-hack "motto of the day"/"motd" welcome message
    mount --bind $SONOFF_HACK_PREFIX/etc/motd /etc/motd
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
    $START_STOP_SCRIPT mqtt start
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
    $START_STOP_SCRIPT rtsp start
fi

if [[ $(get_config ONVIF) == "yes" ]] ; then
    $START_STOP_SCRIPT onvif start

    if [[ $(get_config ONVIF_WSDD) == "yes" ]] ; then
        $START_STOP_SCRIPT wsdd start
    fi
fi

if [[ $(get_config HTTPD) == "yes" ]] ; then
    mkdir -p /tmp/mdns.d
    echo -e "type _http._tcp\nport $HTTPD_PORT\n" > /tmp/mdns.d/http.service
fi
if [[ $(get_config SSHD) == "yes" ]] ; then
    mkdir -p /tmp/mdns.d
    echo -e "type _ssh._tcp\nport 22\n" > /tmp/mdns.d/ssh.service
fi
if [[ $(get_config MDNSD) == "yes" ]] ; then
    # Use wifi mac as unique id
    MAC_ADDR=$(ifconfig ra0 | awk '/HWaddr/{print substr($5,1)}')
    mkdir -p /tmp/mdns.d
    echo -e "type _yi-hack._tcp\nport $HTTPD_PORT\ntxt mac=$MAC_ADDR\n" > /tmp/mdns.d/yi-hack.service
    /mnt/mmc/sonoff-hack/sbin/mdnsd /tmp/mdns.d
fi

# Run rtsp watchdog
$SONOFF_HACK_PREFIX/script/wd.sh &

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
