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

###############################################################################

SCRIPT_DIR=$(get_script_dir)

source "${SCRIPT_DIR}/common.sh"

for CAMERA_NAME in "${!CAMERAS[@]}"; do 
    $SCRIPT_DIR/pack_fw.sh $CAMERA_NAME
done
