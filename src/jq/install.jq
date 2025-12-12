#!/bin/bash

SCRIPT_DIR=$(cd `dirname $0` && pwd)
cd $SCRIPT_DIR

mkdir -p ../../build/sonoff-hack/bin/ || exit 1
mkdir -p ../../build/sonoff-hack/lib/ || exit 1

rsync -av ./_install/* ../../build/sonoff-hack/ || exit 1
