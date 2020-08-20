#!/bin/sh

cd /mnt/mmc/sonoff-hack/etc

rm hostname

rm camera.conf
rm mqtt-sonoff.conf
rm system.conf
rm ptz_presets.conf
rm -r dropbear

tar zxvf defaults.tar.gz > /dev/null 2>&1

printf "Content-type: application/json\r\n\r\n"
printf "{\n"
printf "}\n"
