#!/bin/bash

DIR="${BASH_SOURCE%/*}"
if  ! test -d "$DIR" ; then
	DIR="$PWD"
fi
SIPI_MAIN="$(dirname "$DIR")"
SIPI_BIN="$SIPI_MAIN/local/bin/sipi"
SIPI_CONFIG="$SIPI_MAIN/config/sipi.test-config.lua"

$("$SIPI_BIN -config $SIPI_CONFIG") &

PID_SIPI=$!

. "$DIR/test_jpeg.sh"

