#!/bin/bash

#
#  This file is part of sonoff-hack.
#  Copyright (c) 2019 densanki.
#  Copyright (c) 2019 Davide Maggioni.
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

#set -e

get_script_dir()
{
    echo "$(cd `dirname $0` && pwd)"
}

source "$(get_script_dir)/common.sh"

echo ""
echo "------------------------------------------------------------------------"
echo " SONOFF-HACK - CLEANUP"
echo "------------------------------------------------------------------------"
echo ""

BASE_DIR=$(get_script_dir)/../
BASE_DIR=$(normalize_path $BASE_DIR)

STATIC_DIR=$BASE_DIR/static
BUILD_DIR=$BASE_DIR/build
OUT_DIR=$BASE_DIR/out

echo "Cleaning build dir ..."
rm -rf $BUILD_DIR
echo "Cleaning out dir ..."
rm -rf $OUT_DIR

echo "Cleaning src/*/_install folders ..."

SRC_DIR=$(get_script_dir)/../src

for SUB_DIR in $SRC_DIR/* ; do
    if [ -d ${SUB_DIR} ]; then # Will not run if no directories are available
        echo -n "Cleaning _install in $(basename \"$SUB_DIR\") ..."
        rm -rf $SUB_DIR/_install
        echo "done!"
    fi
done

echo ""

for SUB_DIR in $SRC_DIR/* ; do
    if [ -d ${SUB_DIR} ]; then # Will not run if no directories are available
        echo -n "Cleaning $(basename \"$SUB_DIR\") ..."
        cd $SUB_DIR
        MOD_DIR=$(basename $SUB_DIR)
        ./cleanup.$MOD_DIR
        echo "done!"
    fi
done

echo ""
echo "Finished!"
echo ""
