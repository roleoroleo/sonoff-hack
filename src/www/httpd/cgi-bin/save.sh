#!/bin/sh

printf "Content-type: application/octet-stream\r\n\r\n"

TMP_DIR="/tmp/sonoff-temp-save"
mkdir $TMP_DIR
cd $TMP_DIR
cp /mnt/mmc/sonoff-hack/etc/*.conf .
if [ -f /mnt/mmc/sonoff-hack/etc/hostname ]; then
    cp /mnt/mmc/sonoff-hack/etc/hostname .
fi
tar zcvf config.tar.gz * > /dev/null
cat $TMP_DIR/config.tar.gz
cd /tmp
rm -rf $TMP_DIR
