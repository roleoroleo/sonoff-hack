#!/bin/sh

removedoublequotes(){
  echo "$(sed 's/^"//g;s/"$//g')"
}

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"
WPA_FILE="/mnt/mtd/ipc/cfg/wpa_supplicant.conf"

. $SONOFF_HACK_PREFIX/www/cgi-bin/validate.sh

if ! $(validateQueryString $QUERY_STRING); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

ACTION="none"

PARAM="$(echo $QUERY_STRING | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"
PWD=""
PWD2=""

if [ "$PARAM" == "action" ]; then
     ACTION=$VAL
fi

if [ $ACTION == "scan" ]; then

    printf "Content-type: application/json\r\n\r\n"
    printf "{\"wifi\":[\n"

    LIST=`iwlist ra0 scan | grep "ESSID:" | cut -d : -f 2,3,4,5,6,7,8 | grep -v -e '^$'`

    IFS="\""
    for l in $LIST; do
        if [ ! -z $(echo $l | tr -d ' ') ]; then
            printf "\"$l\", \n"
        fi
    done

    printf "\"\"]}\n"

elif [ $ACTION == "save" ]; then

    read -r POST_DATA

    # Validate json
    VALID=$(echo "$POST_DATA" | jq -e . >/dev/null 2>&1; echo $?)
    if [ "$VALID" != "0" ]; then
        printf "Content-type: application/json\r\n\r\n"
        printf "{\n"
        printf "\"%s\":\"%s\"\\n" "error" "true"
        printf "}"
    fi
    KEYS=$(echo "$POST_DATA" | jq keys_unsorted[])
    for KEY in $KEYS; do
        KEY=$(echo $KEY | removedoublequotes)
        VALUE=$(echo "$POST_DATA" | jq -r .$KEY)
        if [ $KEY == "WIFI_ESSID" ]; then
            ESSID=$VALUE
        elif [ $KEY == "WIFI_PASSWORD" ]; then
            PWD=$VALUE
        elif [ $KEY == "WIFI_PASSWORD2" ]; then
            PWD2=$VALUE
        fi
    done

    if [ "$PWD" == "$PWD2" ]; then
        /mnt/mmc/sonoff-hack/bin/sqlite3 /mnt/mtd/db/ipcsys.db \
            "update t_sys_param set c_param_value='$ESSID' where c_param_name='wf_ssid'; \
            update t_sys_param set c_param_value='$PWD' where c_param_name='wf_key';
            update t_sys_param set c_param_value='1' where c_param_name='wf_status'; \
            update t_sys_param set c_param_value='3' where c_param_name='wf_auth'; \
            update t_sys_param set c_param_value='1' where c_param_name='wf_enc';"

        if [ -f $WPA_FILE ]; then
            echo "ctrl_interface=/var/run/wpa_supplicant" > $WPA_FILE
            echo "update_config=1" >> $WPA_FILE
            echo "network={" >> $WPA_FILE
            echo -e "\tssid=\"$ESSID\"" >> $WPA_FILE
            echo -e "\tscan_ssid=1" >> $WPA_FILE
            echo -e "\tkey_mgmt=WPA-EAP WPA-PSK IEEE8021X NONE" >> $WPA_FILE
            echo -e "\tpairwise=TKIP CCMP" >> $WPA_FILE
            echo -e "\tgroup=CCMP TKIP WEP104 WEP40" >> $WPA_FILE
            echo -e "\tpsk=\"$PWD\"" >> $WPA_FILE
            echo "}" >> $WPA_FILE
        fi

        printf "Content-type: application/json\r\n\r\n"
        printf "{\n"
        printf "\"%s\":\"%s\"\\n" "error" "false"
        printf "}"
    else
        printf "Content-type: application/json\r\n\r\n"
        printf "{\n"
        printf "\"%s\":\"%s\"\\n" "error" "true"
        printf "}"
    fi

fi
