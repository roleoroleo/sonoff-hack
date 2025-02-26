#!/bin/sh

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

sedencode(){
#  echo -e "$(sed 's/\\/\\\\\\/g;s/\&/\\\&/g;s/\//\\\//g;')"
  echo "$(sed 's/\\/\\\\/g;s/\"/\\\"/g;s/\&/\\\&/g;s/\//\\\//g;')"
}

removedoublequotes(){
  echo "$(sed 's/^"//g;s/"$//g')"
}

get_conf_type()
{
    CONF="$(echo $QUERY_STRING | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

    if [ $CONF == "conf" ] ; then
        echo $VAL
    fi
}

. $SONOFF_HACK_PREFIX/www/cgi-bin/validate.sh

if ! $(validateQueryString $QUERY_STRING); then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit
fi

CONF_TYPE="$(get_conf_type)"
CONF_FILE=""

if [ "$CONF_TYPE" == "mqtt" ] ; then
    CONF_FILE="$SONOFF_HACK_PREFIX/etc/mqtt-sonoff.conf"
else
    CONF_FILE="$SONOFF_HACK_PREFIX/etc/$CONF_TYPE.conf"
fi

USERNAME="none"
PASSWORD="none"
TIMEZONE="none"

read -r POST_DATA
# Validate json
VALID=$(echo "$POST_DATA" | jq -e . >/dev/null 2>&1; echo $?)
if [ "$VALID" != "0" ]; then
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
fi
# Change temporarily \n with \t (2 bytes)
POST_DATA=$(echo "$POST_DATA" | sed 's/\\n/\\t/g')
IFS=$(echo -en "\n\b")
ROWS=$(echo "$POST_DATA" | jq -r '. | keys[] as $k | "\($k)=\(.[$k])"')
for ROW in $ROWS; do
    ROW=$(echo "$ROW" | removedoublequotes)
    KEY=$(echo "$ROW" | cut -d'=' -f1)
    # Change back tab with \n
    VALUE=$(echo "$ROW" | cut -d'=' -f2 | sed 's/\t/\\n/g')

    if ! $(validateKey $KEY); then
        printf "Content-type: application/json\r\n\r\n"
        printf "{\n"
        printf "\"%s\":\"%s\"\\n" "error" "true"
        printf "}"
        exit
    fi

    if [ "$KEY" == "HOSTNAME" ] ; then
        if [ -z $VALUE ] ; then

            # Use 2 last MAC address numbers to set a different hostname
            MAC=$(cat /sys/class/net/ra0/address|cut -d ':' -f 5,6|sed 's/://g')
            if [ "$MAC" != "" ]; then
                hostname sonoff-$MAC
            else
                hostname sonoff-hack
            fi
            hostname > $SONOFF_HACK_PREFIX/etc/hostname
        else
            hostname $VALUE
            echo "$VALUE" > $SONOFF_HACK_PREFIX/etc/hostname
        fi
    else
        if [ "$KEY" == "USERNAME" ] ; then
            VALUE=$(echo "$VALUE" | sedencode)
            sed -i "s/^\(${KEY}\s*=\s*\).*$/\1${VALUE}/" $CONF_FILE
            USERNAME=$VALUE
        elif [ "$KEY" == "PASSWORD" ] ; then
            VALUE=$(echo "$VALUE" | sedencode)
            sed -i "s/^\(${KEY}\s*=\s*\).*$/\1${VALUE}/" $CONF_FILE
            PASSWORD=$VALUE
        elif [ "$KEY" == "TIMEZONE" ] ; then
            # Don't save timezone
            TIMEZONE=$VALUE
        elif [ "$KEY" == "MOTION_IMAGE_DELAY" ] ; then
            if $(validateNumber $VALUE); then
                VALUE=$(echo $VALUE | sed 's/,/./g')
                VAR=$(awk 'BEGIN{ print "'$VALUE'"<="'5.0'" }')
                if [ "$VAR" == "1" ]; then
                    sed -i "s/^\(${KEY}\s*=\s*\).*$/\1${VALUE}/" $CONF_FILE
                fi
            fi
        else
            KEY=$(echo "$KEY" | sedencode)
            VALUE=$(echo "$VALUE" | sedencode)
            sed -i "s/^\(${KEY}\s*=\s*\).*$/\1${VALUE}/" $CONF_FILE
        fi
    fi

done

if [ "$CONF_TYPE" == "system" ] && [ x$USERNAME != "xnone" ] ; then
    # Add username and password to t_user table
    # If we don't add user now we need 2 reboots
    SQL_USER=$(sqlite3 /mnt/mtd/db/ipcsys.db "select C_UserName from t_user where C_UserID=10101;")
    SQL_PWD=$(sqlite3 /mnt/mtd/db/ipcsys.db "select C_PassWord from t_user where C_UserID=10101;")

    if [ x$SQL_USER != x$USERNAME ] || [ x$SQL_PWD != x$PASSWORD ]; then
        sqlite3 /mnt/mtd/db/ipcsys.db "delete from t_user where C_UserID=10101;"
        if [[ x$USERNAME != "x" ]] ; then
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
insert into t_user (C_UserID, c_role_id, C_UserName, C_PassWord) values (10101, 1, '$USERNAME', '$PASSWORD');
EOF
        else
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
insert into t_user (C_UserID, c_role_id, C_UserName, C_PassWord) values (10101, 1, 'hack', 'hack');
EOF
        fi
    fi
fi

if [ "$CONF_TYPE" == "system" ] && [ x$TIMEZONE != "xnone" ] ; then
    # Add timezone settings to t_sys_param table
    TZS=$(sqlite3 /mnt/mtd/db/ipcsys.db "select * from t_zonetime_info;")
    COUNTER=1
    for TZR in $TZS; do
        TZR1=$(echo $TZR | cut -d"|" -f1)
        if [ "$TZR1" == "$TIMEZONE" ]; then
            sqlite3 /mnt/mtd/db/ipcsys.db <<EOF &
.timeout 3000
update t_sys_param set c_param_value='$COUNTER' where c_param_name='ZoneTimeName';
EOF
            break
        fi
        let COUNTER=COUNTER+1
    done

    # Add timezone to config_timezone.ini file
    CONFIG_TIMEZONE=/mnt/mtd/ipc/cfg/config_timezone.ini
    if [ -f /dayun/mtd/db/conf/config_timezone.ini ]; then
        CONFIG_TIMEZONE=/dayun/mtd/db/conf/config_timezone.ini
    fi

    TZP=$(sqlite3 /mnt/mtd/db/ipcsys.db "select c_zonetime_value from t_zonetime_info where c_zonetime_name='$TIMEZONE';" | sed 's/GMT//g' | sed 's/://g')
    TZP_SET=$(echo ${TZP:0:1} ${TZP:1:2} ${TZP:3:2} | awk '{ print ($1$2*3600+$3*60) }' | sed 's/^+//g')
    TZP_CUR=$(cat $CONFIG_TIMEZONE | grep offset_second= | sed 's/offset_second=//g' | sed 's/\"//g')
    if [ "$TZP_SET" != "$TZP_CUR" ]; then
        sed "s/offset_second=\"$TZP_CUR\"/offset_second=\"$TZP_SET\"/g" -i $CONFIG_TIMEZONE
    fi

fi

# Yeah, it's pretty ugly.

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
