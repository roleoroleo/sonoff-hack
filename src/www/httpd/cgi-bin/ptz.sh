#!/bin/sh

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

. $SONOFF_HACK_PREFIX/www/cgi-bin/validate.sh

if ! $(validateQueryString $QUERY_STRING); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

DIR="none"

for I in 1 2
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "dir" ] ; then
        X=0
        Y=0
        N_STEP=-200
        P_STEP=200
        if [ "$VAL" == "right" ]; then
            X=$N_STEP
        elif [ "$VAL" == "left" ]; then
            X=$P_STEP
        elif [ "$VAL" == "up" ]; then
            Y=$P_STEP
        elif [ "$VAL" == "down" ]; then
            Y=$N_STEP
        elif [ "$VAL" == "up_right" ]; then
            X=$N_STEP
            Y=$P_STEP
        elif [ "$VAL" == "up_left" ]; then
            X=$P_STEP
            Y=$P_STEP
        elif [ "$VAL" == "down_right" ]; then
            X=$N_STEP
            Y=$N_STEP
        elif [ "$VAL" == "down_left" ]; then
            X=$P_STEP
            Y=$N_STEP
        fi
        # For mirrored movements negate values
        if [ -f /tmp/.mirror ]; then
            X=$(( 0 - $X ))
            Y=$(( 0 - $Y ))
        fi
        DIR="-x $X -y $Y"
    fi  
done

if ! $(validateString $DIR); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\",\\n" "error" "true"
    printf "\"%s\":\"%s\"\\n" "description" "wrong dir parameter"
    printf "}"
    exit
fi

if [ "$DIR" != "none" ] ; then
ptz -a go_rel $DIR      
fi

printf "Content-type: application/json\r\n\r\n"
printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
