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

get_script_dir()
{
    echo "$(cd `dirname $0` && pwd)"
}

SCRIPT_DIR=$(get_script_dir)

source "${SCRIPT_DIR}/common.sh"

echo ""
echo "------------------------------------------------------------------------"
echo " SONOFF-HACK - SRC COMPILER"
echo "------------------------------------------------------------------------"
echo ""

mkdir -p "${SCRIPT_DIR}/../build/sonoff-hack"
SRC_DIR=${SCRIPT_DIR}/../src

if [ ! -z "$1" ]; then 
    echo "=============================== MODULE $1 ====================="
	compile_module $(normalize_path "${SRC_DIR}/$1") || exit 1
else
    for SUB_DIR in ${SRC_DIR}/* ; do
    if [ -d ${SUB_DIR} ]; then # Will not run if no directories are available
        echo "=============================== MODULE ${SUB_DIR} ====================="
        compile_module $(normalize_path "${SUB_DIR}") || exit 1
    fi
    done
fi
