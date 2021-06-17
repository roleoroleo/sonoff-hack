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
# Change temporarily \n with \t (2 bytes)
POST_DATA=$(echo "$POST_DATA" | sed 's/\\n/\\t/g')
IFS=$(echo -en "\n\b")
ROWS=$(echo "$POST_DATA" | jq -r '. | keys[] as $k | "\($k)=\(.[$k])"')
for ROW in $ROWS; do
    ROW=$(echo "$ROW" | removedoublequotes)
    KEY=$(echo "$ROW" | cut -d'=' -f1)
    # Change back tab with \n
    VALUE=$(echo "$ROW" | cut -d'=' -f2 | sed 's/\t/\\n/g')

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
        else
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
            sqlite3 /mnt/mtd/db/ipcsys.db "insert into t_user (C_UserID, c_role_id, C_UserName, C_PassWord) values (10101, 1, '$USERNAME', '$PASSWORD');"
        else
            sqlite3 /mnt/mtd/db/ipcsys.db "insert into t_user (C_UserID, c_role_id, C_UserName, C_PassWord) values (10101, 1, 'hack', 'hack');"
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
            sqlite3 /mnt/mtd/db/ipcsys.db "update t_sys_param set c_param_value='$COUNTER' where c_param_name='ZoneTimeName';"
            break
        fi
        let COUNTER=COUNTER+1
    done
fi

# Yeah, it's pretty ugly.

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
