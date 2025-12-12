#!/bin/bash

. ./config.jq

SCRIPT_DIR=$(cd `dirname $0` && pwd)
cd $SCRIPT_DIR

rm -rf ./_install

if [ ! -f $ARCHIVE ]; then
    wget https://github.com/stedolan/jq/releases/download/jq-${VERSION}/$ARCHIVE
fi
tar zxvf $ARCHIVE

cd jq-${VERSION} || exit 1

export CFLAGS+="-Os -ffunction-sections -fdata-sections"
export LDFLAGS+="-Wl,--gc-sections"
./configure --host=arm-sonoff-linux-uclibcgnueabi --disable-docs
sed -i 's/-DHAVE_J0=1//g' Makefile
sed -i 's/-DHAVE_J1=1//g' Makefile
sed -i 's/-DHAVE_Y0=1//g' Makefile
sed -i 's/-DHAVE_Y1=1//g' Makefile
