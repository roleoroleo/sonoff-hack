#!/bin/sh

echo "############## Starting Hack ##############"

touch /tmp/di.wav
touch /tmp/Internet_connected_Welcome_to_use_cloud_camera.wav
touch /tmp/WiFi_connect_success.wav

mount --bind /tmp/di.wav /mnt/mtd/ipc/app/res/En/di.wav
mount --bind /tmp/Internet_connected_Welcome_to_use_cloud_camera.wav /mnt/mtd/ipc/app/res/En/Internet_connected_Welcome_to_use_cloud_camera.wav
mount --bind /tmp/WiFi_connect_success.wav /mnt/mtd/ipc/app/res/En/WiFi_connect_success.wav

(sleep 30 && /mnt/mmc/sonoff-hack/script/system.sh) &
