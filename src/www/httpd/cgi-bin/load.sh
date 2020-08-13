#!/bin/sh

# Conf
CONF_FILE="etc/camera.conf"
SONOFF_HACK_PREFIX="/mnt/mmc/sonoff-hack"

get_config()
{
    key=$1
    grep -w $1 $SONOFF_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2
}

# Files
TMPOUT=/tmp/config.tar.gz.dl
TMPDIR=/tmp/workdir.tmp
TMPOUTgz=$TMPDIR/config.tar.gz

# Cleaning
rm -f $TMPOUT
rm -f $TMPOUTgz
rm -rf $TMPDIR

mkdir -p $TMPDIR

if [ $CONTENT_LENGTH -gt 10000 ]; then
    exit
fi

if [ "$REQUEST_METHOD" = "POST" ]; then
    cat >$TMPOUT

    # Get the line count
    LINES=$(grep -c "" $TMPOUT)

    touch $TMPOUTgz
    l=1
    LENSKIP=0

    # Process post data removing head and tail
    while true; do
        if [ $l -eq 1 ]; then
            ROW=`cat $TMPOUT | awk "FNR == $l {print}"`
            BOUNDARY=${#ROW}
            BOUNDARY=$((BOUNDARY+1))
            LENSKIPSTART=$BOUNDARY
            LENSKIPEND=$BOUNDARY
        elif [ $l -le 4 ]; then
            ROW=`cat $TMPOUT | awk "FNR == $l {print}"`
            ROWLEN=${#ROW}
            LENSKIPSTART=$((LENSKIPSTART+ROWLEN+1))
        elif [ \( $l -gt 4 \) -a \( $l -lt $LINES \) ]; then
            ROW=`cat $TMPOUT | awk "FNR == $l {print}"`
        else
            break
        fi
        l=$((l+1))
    done
fi

# Extract tar.gz file
LEN=$((CONTENT_LENGTH-LENSKIPSTART-LENSKIPEND+2))
dd if=$TMPOUT of=$TMPOUTgz bs=1 skip=$LENSKIPSTART count=$LEN >/dev/null 2>&1
cd $TMPDIR
tar zxvf $TMPOUTgz >/dev/null 2>&1
RES=$?

# Verify result of tar.bz2 command and copy files to destination
if [ $RES -eq 0 ]; then
    if [ \( -f "system.conf" \) -a \( -f "camera.conf" \) ]; then
        mv -f *.conf /mnt/mmc/sonoff-hack/etc/
        chmod 0644 /mnt/mmc/sonoff-hack/etc/*.conf
        if [ -f hostname ]; then
            mv -f hostname /mnt/mmc/sonoff-hack/etc/
            chmod 0644 /mnt/mmc/sonoff-hack/etc/hostname
        fi
        RES=0
    else
        RES=1
    fi
fi

# Cleaning
cd ..
rm -rf $TMPDIR
rm -f $TMPOUT
rm -f $TMPOUTgz

# Print response
printf "Content-type: text/html\r\n\r\n"
if [ $RES -eq 0 ]; then
    printf "Upload completed successfully, restart your camera\r\n"
else
    printf "Upload failed\r\n"
fi

if [ ! -f "$SONOFF_HACK_PREFIX/$CONF_FILE" ]; then
    exit
fi
