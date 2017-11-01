#!/bin/bash
#
for file in $1/*.tif
do
    if test -f "$file"
    then
	echo "processing $file..."
	/usr/local/bin/sipi --file $file --format jpx ${file%%.*}.jp2 
    fi
done