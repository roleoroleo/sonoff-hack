#!/bin/sh

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

printf "Content-type: text/javascript\r\n\r\n"

printf "hostname=\"%s\";" $(cat $SONOFF_HACK_PREFIX/etc/hostname)
