.. Copyright © 2017 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
   Tobias Schweizer, and Ivan Subotic.
   
   This file is part of Sipi.

   Sipi is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Sipi is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   Additional permission under GNU AGPL version 3 section 7:
   If you modify this Program, or any covered work, by linking or combining
   it with Kakadu (or a modified version of that library) or Adobe ICC Color
   Profiles (or a modified version of that library) or both, containing parts
   covered by the terms of the Kakadu Software Licence or Adobe Software Licence,
   or both, the licensors of this Program grant you additional permission
   to convey the resulting work.

   See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public
   License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.

.. highlight:: none

############
Running Sipi
############

.. contents:: :local:

After following the instructions in :ref:`building`, you will find the executable
``local/bin/sipi`` in the source tree.

It can be run either as simple command-line image converter or as a server.


**********************************************
Running Sipi As a Command-line Image Converter
**********************************************

Convert an image file to another format:

::

   local/bin/sipi --format [output format] --fileIn [input file] [output file]

Compare two image files:

::

   local/bin/sipi --Compare file1 --Compare file2 


************************
Running Sipi As a Server
************************

::

   local/bin/sipi --config [config file]

Sipi logs its operations using syslog_.

********************
Command-line Options
********************

::

   Options:
     --config filename, -c filename
                       Configuration file for web server.

     --file fileIn, -f fileIn
                       input file to be converted. Usage: sipi [options] -f fileIn
                       fileout

     --format Value, -F Value
                       Output format Value can be: jpx,jpg,tif,png.

     --ICC Value, -I Value
                       Convert to ICC profile. Value can be:
                       none,sRGB,AdobeRGB,GRAY.

     --quality Value, -q Value
                       Quality (compression). Value can any integer between 1 and
                       100

     --region x,y,w,h, -r x,y,w,h
                       Select region of interest, where x,y,w,h are integer values

     --Reduce Value, -R Value
                       Reduce image size by factor Value (cannot be used together
                       with --size and --scale)

     --size w,h -s w,h
                       Resize image to given size w,h (cannot be used together with
                       --reduce and --scale)

     --Scale Value, -S Value
                       Resize image by the given percentage Value (cannot be used
                       together with --size and --reduce)

     --skipmeta Value, -k Value
                       Skip the given metadata. Value can be none,all

     --mirror Value, -m Value
                       Mirror the image. Value can be: none,horizontal,vertical

     --rotate Value, -o Value
                       Rotate the image. by degree Value, angle between (0:360)

     --salsah, -s
                       Special flag for SALSAH internal use

     --Compare file1 --Compare file2 or -C file1 -C file2
                       Compare two files

     --watermark file, -w file
                       Add a watermark to the image

     --serverport Value, -p Value
                       Port of the web server

     --nthreads Value, -t Value
                       Number of threads for web server

     --imgroot Value, -i Value
                       Root directory containing the images for the web server

     --loglevel Value, -l Value
                       Logging level Value can be:
                       TRACE,DEBUG,INFO,WARN,ERROR,CRITICAL,OFF

     --help
                       Print usage and exit.


*******************
Configuration Files
*******************

Sipi's configuration file is written in Lua_. You can make your own configuration file by
adapting ``config/sipi.config.lua``.

- Check that the port number is correct and that your operating system's firewal
  does not block it.
- Set ``imgroot`` to the directory containing the files to be served.
- Create the directory ``cache`` in the top-level directory of the source tree.

For more information, see the comments in ``config/sipi.config.lua``.

Using Sipi with Knora
=====================

If you are using Sipi with Knora_, you can adapt ``config/sipi.knora-config.lua``.

HTTPS Support
=============

Sipi supports SSL/TLS encryption if the OpenSSL_ library is installed. You will
need to install a certificate; see ``config/sipi.config.lua`` for instructions.

IIIF Prefixes
=============

Sipi supports `IIIF image URLs`_.

If the configuration property ``prefix_as_path`` is set to ``true``, the IIIF
``prefix`` portion of the URL is interpreted as a subdirectory of ``imgroot``,
and Sipi looks for the requested image file in that subdirectory. Otherwise,
it looks for the file in ``imgroot``.


.. _syslog: http://man7.org/linux/man-pages/man3/syslog.3.html
.. _Lua: https://www.lua.org/
.. _Knora: http://www.knora.org/
.. _OpenSSL: https://www.openssl.org/
.. _IIIF image URLs: http://iiif.io/api/image/2.1/#image-request-uri-syntax
