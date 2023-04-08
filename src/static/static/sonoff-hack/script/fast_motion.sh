#!/bin/sh

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"
MQTT_HOST=192.168.11.5
MQTT_USER=openhab_iot
MQTT_PASS=PASSWORD

#LOG_FILE="/dev/null"
LOG_FILE="/tmp/fastmotion.log"

log() {
	echo "$(date +'%Y-%m-%d %H:%M:%S') -" "$*" >> $LOG_FILE
}
run() {
	${SONOFF_HACK_PREFIX}/usr/bin/inotifyd - /tmp:n | while read a b c; do 
		log received $a $b $c
		if [ "$c" = "colinkPushNotice" ]; then
			${SONOFF_HACK_PREFIX}/bin/mosquitto_pub -h ${MQTT_HOST} -u ${MQTT_USER} -P ${MQTT_PASS} -t cam1/fast_motion -m ON
			${SONOFF_HACK_PREFIX}/bin/snapshot -f /tmp/snapfast.jpg
			${SONOFF_HACK_PREFIX}/bin/mosquitto_pub -h ${MQTT_HOST} -u ${MQTT_USER} -P ${MQTT_PASS} -t cam1/fast_image -f /tmp/snapfast.jpg
			sleep 5
			${SONOFF_HACK_PREFIX}/bin/mosquitto_pub -h ${MQTT_HOST} -u ${MQTT_USER} -P ${MQTT_PASS} -t cam1/fast_motion -m OFF
		fi
	done
}

run_loop() {
	while true; do
		log "Starting fast mation detection ..."
		run
		log "aborted ... waiting 10 seconds"
		sleep 10
	done
}

run_loop
