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

function find_and_delete {

FILENAME_RM=$1

if test -f "$FILENAME_RM"; then
   echo -e "${GREEN} file ${FILENAME_RM} found - I will delete it${NOCOLOR}"
   rm "$FILENAME_RM"
fi
}

./local/bin/sipi -config config/sipi.test-config.lua &

sleep 1.5

PID_SIPI=$!
JPEG_EXTENTION="jpg"
FILENAME_J8_DOWN="test_server/images/tif8tojpeg.jpeg"
FILENAME_J16_DOWN="test_server/images/tif16tojpeg.jpeg"


find_and_delete $FILENAME_J8_DOWN
find_and_delete $FILENAME_J16_DOWN


PNG_EXTENTION="png"
FILENAME_P8_DOWN="test_server/images/tif8topng.png"
FILENAME_P16_DOWN="test_server/images/tif16topng.png"

find_and_delete $FILENAME_P8_DOWN
find_and_delete $FILENAME_P16_DOWN


J2_EXTENTION="jp2"
FILENAME_J28_DOWN="test_server/images/tif8tojp2.jp2"
FILENAME_J216_DOWN="test_server/images/tif16tojp2.jp2"

find_and_delete $FILENAME_J28_DOWN
find_and_delete $FILENAME_J216_DOWN


bash test_server/test_generic.sh $PID_SIPI $FILENAME_J8_DOWN $FILENAME_J16_DOWN $JPEG_EXTENTION
bash test_server/test_generic.sh $PID_SIPI $FILENAME_P8_DOWN $FILENAME_P16_DOWN $PNG_EXTENTION
bash test_server/test_generic.sh $PID_SIPI $FILENAME_J28_DOWN $FILENAME_J216_DOWN $J2_EXTENTION


kill $PID_SIPI

exit $?
