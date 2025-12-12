#!/bin/bash

set -e

. ./config.jq
export CROSSPATH=/home/user/x-tools/arm-sonoff-linux-uclibcgnueabi/bin
export PATH=${PATH}:${CROSSPATH}

export TARGET=arm-sonoff-linux-uclibcgnueabi
export CROSS=arm-sonoff-linux-uclibcgnueabi
export BUILD=x86_64-pc-linux-gnu

export CROSSPREFIX=${CROSS}-

export STRIP=${CROSSPREFIX}strip
export CXX=${CROSSPREFIX}g++
export CC=${CROSSPREFIX}gcc
export LD=${CROSSPREFIX}ld
export AS=${CROSSPREFIX}as
export AR=${CROSSPREFIX}ar

SCRIPT_DIR=$(cd `dirname $0` && pwd)
cd $SCRIPT_DIR

cd jq-${VERSION}

make clean
make -j $(nproc)

mkdir -p ../_install/bin
mkdir -p ../_install/lib

cp ./vendor/oniguruma/src/.libs/libonig.so.5 ../_install/lib
cp ./.libs/libjq.so.1.0.4 ../_install/lib/libjq.so.1
cp ./.libs/jq ../_install/bin

$STRIP ../_install/bin/*
$STRIP ../_install/lib/*
