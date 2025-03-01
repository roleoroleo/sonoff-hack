#!/bin/sh

echo "############## Starting Hack ##############"

# Fix log path
mkdir -p /var/log
mount --bind /var/sdcard/log /var/log

# Remove audio messages during boot
touch /tmp/EmptyAudio.wav

WAV_FILE_DIR=/mnt/mtd/ipc/app/res/En
[ -d /mnt/mtd/ipc/app/snd/english ] && WAV_FILE_DIR=/mnt/mtd/ipc/app/snd/english

mount --bind /tmp/EmptyAudio.wav $WAV_FILE_DIR/di.wav
mount --bind /tmp/EmptyAudio.wav $WAV_FILE_DIR/Internet_connected_Welcome_to_use_cloud_camera.wav
mount --bind /tmp/EmptyAudio.wav $WAV_FILE_DIR/WiFi_connect_success.wav
mount --bind /tmp/EmptyAudio.wav $WAV_FILE_DIR/Please_use_mobile_phone_for_WiFi_configuration.wav

# Initialize WiFi               
WPA_FILE="/mnt/mtd/ipc/cfg/wpa_supplicant.conf"
WPA_INIT_FILE="/var/sdcard/wifi_init.txt"
cp $WPA_FILE /var/sdcard/
if [ -f $WPA_INIT_FILE ]; then
    echo "init wifi" >> /var/log/boot.log
    # Load variables WIFI_ESSID and WIFI_KEY from ini file
    . $WPA_INIT_FILE
    
    # Check for required variables WIFI_ESSID and WIFI_KEY
    if [ -n "$WIFI_ESSID" ] || [ -n "$WIFI_KEY" ]; then
        /var/sdcard/sonoff-hack/bin/sqlite3 /mnt/mtd/db/ipcsys.db \
           "update t_sys_param set c_param_value='$WIFI_ESSID' where c_param_name='wf_ssid'; \
            update t_sys_param set c_param_value='$WIFI_KEY' where c_param_name='wf_key';
            update t_sys_param set c_param_value='1' where c_param_name='wf_status'; \
            update t_sys_param set c_param_value='3' where c_param_name='wf_auth'; \
            update t_sys_param set c_param_value='1' where c_param_name='wf_enc';"

        cat << EOF > $WPA_FILE
ctrl_interface=/var/run/wpa_supplicant
update_config=1
network={
	ssid="$WIFI_ESSID"
	scan_ssid=1
	key_mgmt=WPA-EAP WPA-PSK IEEE8021X NONE
	pairwise=TKIP CCMP
	group=CCMP TKIP WEP104 WEP40
	psk="$WIFI_KEY"
}
EOF
        mv $WPA_INIT_FILE ${WPA_INIT_FILE}.done
    else
        mv $WPA_INIT_FILE ${WPA_INIT_FILE}.failed
    fi
fi

# Add script for network management
cp /mnt/mtd/ipc/app/script/dhcp.sh /tmp/dhcp.sh
sed -i "s~udhcpc -a.*~/mnt/mmc/network.sh \$1~g" /tmp/dhcp.sh

# Make wifi work, even if not configured over eWeLink App
# Check for DyVoiceRecog or wpa_supplicant file
sed -i "s~\(DyVoiceRecog\.bin\)~\1\" ] || [ -f \"$WPA_FILE~g" /tmp/dhcp.sh
mount --bind /tmp/dhcp.sh /mnt/mtd/ipc/app/script/dhcp.sh

# Remove colink binary
touch /tmp/colink
mount --bind /tmp/colink /mnt/mtd/ipc/app/colink

(sleep 30 && /mnt/mmc/sonoff-hack/script/system.sh) &
