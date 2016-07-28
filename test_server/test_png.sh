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
