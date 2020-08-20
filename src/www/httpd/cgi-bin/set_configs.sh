#!/bin/sh

SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

#urldecode() { : "${*//+/ }"; echo -e "${_//%/\\x}"; }

urldecode(){
  echo -e "$(sed 's/+/ /g;s/%\(..\)/\\x\1/g;')"
}

sedencode(){
  echo -e "$(sed 's/\\/\\\\\\/g;s/\&/\\\&/g;s/\//\\\//g;')"
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

read POST_DATA

PARAMS=$(echo "$POST_DATA" | tr "\n\r" " " | tr -d " " | sed 's/{\"//g' | sed 's/\"}//g' | sed 's/\",\"/ /g' | sed 's/\":\"/=/g')
USERNAME="none"
PASSWORD="none"

for S in $PARAMS ; do
    PARAM=$(echo "$S" | tr "=" " ")
    KEY=""
    VALUE=""

    for SP in $PARAM ; do
        if [ -z $KEY ]; then
            KEY=$SP
        else
            VALUE=$SP
            VALUE=$(echo "$SP" | urldecode)
        fi
    done

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
        VALUE=$(echo "$VALUE" | sedencode)
        sed -i "s/^\(${KEY}\s*=\s*\).*$/\1${VALUE}/" $CONF_FILE

        if [ "$KEY" == "USERNAME" ] ; then
            USERNAME=$VALUE
        elif [ "$KEY" == "PASSWORD" ] ; then
            PASSWORD=$VALUE
        fi
    fi

done

if [ "$CONF_TYPE" == "system" ] && [ x$USERNAME != "none" ]; then
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

# Yeah, it's pretty ugly.

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
