#!/bin/sh

cd /mnt/mmc/sonoff-hack/etc

rm hostname

rm camera.conf
rm mqttv4.conf
#rm proxychains.conf
rm system.conf

tar zxvf defaults.tar.gz > /dev/null 2>&1

printf "Content-type: application/json\r\n\r\n"
printf "{\n"
printf "}\n"
