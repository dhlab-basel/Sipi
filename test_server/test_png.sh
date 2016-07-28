#
# Copyright © 2016 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
# Ivan Subotic, Tobias Schweizer, André Kilchenmann, and André Fatton.
# This file is part of Sipi.
# Sipi is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# Sipi is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# Additional permission under GNU AGPL version 3 section 7:
# If you modify this Program, or any covered work, by linking or combining
# it with Kakadu (or a modified version of that library) or Adobe ICC Color
# Profiles (or a modified version of that library) or both, containing parts
# covered by the terms of the Kakadu Software Licence or Adobe Software Licence,
# or both, the licensors of this Program grant you additional permission
# to convey the resulting work.
# See the GNU Affero General Public License for more details.
# You should have received a copy of the GNU Affero General Public
# License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.
#
#!/bin/bash

function test_check {
RED='\033[0;31m'
GREEN='\033[0;32m'
NOCOLOR='\033[0m'
FILENAME_1=$2

if ps -p $1 > /dev/null ;then

    if test -f "$FILENAME_1"; then
        echo -e "${GREEN} file $FILENAME_1 created${NOCOLOR}"
        rm "$FILENAME_1"

    else
        echo -e "${RED} file $FILENAME_1 not created${NOCOLOR}"
    fi
else
    echo -e "${RED}SIPI Crashed${NOCOLOR}"
fi
}


PID_SIPI=$1

FILENAME_DOWN="images/tif8topng.png"

echo "$PID_SIPI"

curl -sS -o "$FILENAME_DOWN" -O http://localhost:1024/test_server/Leaves8.tif/full/full/0/default.jpg

test_check $1 $FILENAME_DOWN

FILENAME_DOWN="images/tif16topng.png"

curl -sS -o "$FILENAME_DOWN" -O http://localhost:1024/test_server/Leaves16.tif/full/full/0/default.jpg

test_check $1 $FILENAME_DOWN
