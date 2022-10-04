#!/bin/bash

#
#  This file is part of sonoff-hack.
#  Copyright (c) 2018-2019 Davide Maggioni.
#  Copyright (c) 2020 roleo.
# 
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, version 3.
# 
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#  General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program. If not, see <http://www.gnu.org/licenses/>.
#

###############################################################################
# Cameras list
###############################################################################

declare -A CAMERAS

CAMERAS["GK-200MP2B"]="GK-200MP2B"
CAMERAS["GK-200MP2C"]="GK-200MP2C"
CAMERAS["GK-200MP2-B"]="GK-200MP2-B"

###############################################################################
# Common functions
###############################################################################

if [ -d /home/user/x-tools/arm-sonoff-linux-uclibcgnueabi ]; then
    export PATH="/home/user/x-tools/arm-sonoff-linux-uclibcgnueabi/bin:$PATH"
fi
export CROSSLINK_BASE=$(dirname $(dirname $(which arm-sonoff-linux-uclibcgnueabi-gcc)))

require_root()
{
    if [ "$(whoami)" != "root" ]; then
        echo "$0 must be run as root!"
        exit 1
    fi
}

normalize_path()
{
    local path=${1//\/.\//\/}
    local npath=$(echo $path | sed -e 's;[^/][^/]*/\.\./;;')
    while [[ $npath != $path ]]; do
        path=$npath
        npath=$(echo $path | sed -e 's;[^/][^/]*/\.\./;;')
    done
    echo $path
}

check_camera_name()
{
    local CAMERA_NAME=$1
    if [[ ! ${CAMERAS[$CAMERA_NAME]+_} ]]; then
        printf "%s not found.\n\n" $CAMERA_NAME

        printf "Here's the list of supported cameras:\n\n"
        print_cameras_list 
        printf "\n"
        exit 1
    fi
}

print_cameras_list()
{
    for CAMERA_NAME in "${!CAMERAS[@]}"; do 
        printf "%s\n" $CAMERA_NAME
    done
}

get_camera_id()
{
    local CAMERA_NAME=$1
    check_camera_name $CAMERA_NAME
    echo ${CAMERAS[$CAMERA_NAME]}
}

compile_module()
{
    (
    local MOD_DIR=$1
    local MOD_NAME=$(basename "$MOD_DIR")

    local MOD_INIT="init.$MOD_NAME"
    local MOD_COMPILE="compile.$MOD_NAME"
    local MOD_INSTALL="install.$MOD_NAME"

    printf "MOD_DIR:        %s\n" "$MOD_DIR"
    printf "MOD_NAME:       %s\n" "$MOD_NAME"
    printf "MOD_INIT:       %s\n" "$MOD_INIT"
    printf "MOD_COMPILE:    %s\n" "$MOD_COMPILE"
    printf "MOD_INSTALL:    %s\n" "$MOD_INSTALL"

    cd "$MOD_DIR"

    if [ ! -f $MOD_INIT ]; then
        echo "$MOD_INIT not found.. exiting."
        exit 1
    fi
    if [ ! -f $MOD_COMPILE ]; then
        echo "$MOD_COMPILE not found.. exiting."
        exit 1
    fi
    if [ ! -f $MOD_INSTALL ]; then
        echo "$MOD_INSTALL not found.. exiting."
        exit 1
    fi

    echo ""

    printf "Initializing $MOD_NAME...\n\n"
    ./$MOD_INIT || exit 1

    printf "Compiling $MOD_NAME...\n\n"
    ./$MOD_COMPILE || exit 1

    printf "Installing '$MOD_INSTALL' in the firmware...\n\n"
    ./$MOD_INSTALL || exit 1

    printf "\n\nDone!\n\n"
    )
}
