#!/bin/sh

# Use. Available variables:
# "oldness" variable. Defines which video to retrieve. 
# - "0" (default) or "1" means latest already available.
# - greater than "0" specifies the oldness, so "3" means the third oldest video. Only looks into the current hour folder. So if value is greater than the videos in the latest hour folder, the last one will be sent.
# "type" variable. Defines what to retrieve.
# - "1" (default). Gets relative name in the format of DIR/VIDEO.mp4
# - "2". Gets full URL video.
# - "3". Gets the video itself as video/mp4 inline.
# - "4". Gets the video itself as video/mp4 attachment.
# Examples of use:
# http://IP:PORT/cgi-bin/getlastrecordedvideo.sh?oldness=0&type=4 -- Send the last video as an attachment.
# http://IP:PORT/cgi-bin/getlastrecordedvideo.sh?oldness=0&type=3 -- Show the last video inline in the browser.
# http://IP:PORT/cgi-bin/getlastrecordedvideo.sh?type=1            -- Send the relative route of the last available video.
# http://IP:PORT/cgi-bin/getlastrecordedvideo.sh?oldness=2&type=2  -- Send the URL of the second to last available video.

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

. $SONOFF_HACK_PREFIX/www/cgi-bin/validate.sh

if ! $(validateQueryString $QUERY_STRING); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

CONF_LAST="CONF_LAST"
OLDNESS=0
TYPE=1

for I in 1 2
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "$CONF_LAST" ]; then
        continue
    fi
    CONF_LAST=$CONF

    if [ "$CONF" == "oldness" ] ; then
        OLDNESS=$VAL
    elif [ "$CONF" == "type" ] ; then
        TYPE=$VAL
    fi
done

L_FILE_LIST=`find /mnt/mmc/alarm_record -mindepth 1 -type f \( -name "*.mp4" \) | sort -k 1 -n -r | sed 's/\/mnt\/mmc\/alarm_record\///g'`
COUNT=`find /mnt/mmc/alarm_record -mindepth 1 -type f \( -name "*.mp4" \) | grep ^ -c`
IDX=1
for f in $L_FILE_LIST; do
    if [ ${#f} == 33 ]; then
        VIDNAME="$f"
        if [ "$IDX" -ge "$OLDNESS" ]; then
            break;
        elif [ "$IDX" == "$COUNT" ]; then
            break;
        fi
        IDX=$(($IDX+1))
    fi
done

if [ "$TYPE" == "2" ]; then
    LOCAL_IP=$(ifconfig eth0 | awk '/inet addr/{print substr($2,6)}')
    if [ -z $LOCAL_IP ]; then
        LOCAL_IP=$(ifconfig ra0 | awk '/inet addr/{print substr($2,6)}')
    fi
    source /mnt/mmc/sonoff-hack/etc/system.conf

    printf "Content-type: text/plain\r\n\r\n"
    echo "http://$LOCAL_IP:$HTTPD_PORT/alarm_record/$VIDNAME"
elif [ "$TYPE" == "3" ]; then
    printf "Content-type: video/mp4; charset=utf-8\r\nContent-Disposition: inline; filename=\"$VIDNAME\"\r\n\r\n"
    cat /mnt/mmc/alarm_record/$VIDNAME
    exit
elif [ "$TYPE" == "4" ]; then
    printf "Content-type: video/mp4; charset=utf-8\r\nContent-Disposition: attachment; filename=\"$VIDNAME\"\r\n\r\n"
    cat /mnt/mmc/alarm_record/$VIDNAME
    exit
else
    # Default and type=1
    printf "Content-type: text/plain\r\n\r\n"
    echo "$VIDNAME"
    exit
fi
