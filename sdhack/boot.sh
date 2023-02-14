#!/bin/sh

echo "############## Starting Hack ##############"

# Fix log path
mkdir -p /var/log
mount --bind /var/sdcard/log /var/log

# Remove audio messages during boot
touch /tmp/di.wav
touch /tmp/Internet_connected_Welcome_to_use_cloud_camera.wav
touch /tmp/WiFi_connect_success.wav

WAV_FILE_DIR=/mnt/mtd/ipc/app/res/En
[ -d /mnt/mtd/ipc/app/snd/english ] && WAV_FILE_DIR=/mnt/mtd/ipc/app/snd/english

mount --bind /tmp/di.wav $WAV_FILE_DIR/di.wav
mount --bind /tmp/Internet_connected_Welcome_to_use_cloud_camera.wav $WAV_FILE_DIR/Internet_connected_Welcome_to_use_cloud_camera.wav
mount --bind /tmp/WiFi_connect_success.wav $WAV_FILE_DIR/WiFi_connect_success.wav

# Add script for network management
cp /mnt/mtd/ipc/app/script/dhcp.sh /tmp/dhcp.sh
sed -i "s/udhcpc -a.*/\/mnt\/mmc\/network.sh \$1/g" /tmp/dhcp.sh
mount --bind /tmp/dhcp.sh /mnt/mtd/ipc/app/script/dhcp.sh

# Remove colink binary
touch /tmp/colink
mount --bind /tmp/colink /mnt/mtd/ipc/app/colink

(sleep 30 && /mnt/mmc/sonoff-hack/script/system.sh) &
