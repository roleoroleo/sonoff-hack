#!/bin/sh

#Dynamic IP
HOSTNAME=$(cat /var/sdcard/sonoff-hack/etc/hostname)
udhcpc -a -b -i $1 -x hostname:$HOSTNAME -p /var/dhcp.pid  > /var/dhcp.info

#Static IP
#ifconfig eth0 192.168.0.150 netmask 255.255.255.0
#route add default gw 192.168.0.254
#echo "nameserver 208.67.222.222" >/etc/resolv.conf
#echo "nameserver 208.67.220.220" >>/etc/resolv.conf
