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

########
Overview
########

Sipi is a high-performance media server developed by the
`Digital Humanities Lab`_ at the `University of Basel`_. It is designed to
be used by archives, libraries, and other institutions that need to preserve
high-quality images while making them available online. 

Sipi implements the International Image Interoperability Framework (IIIF_),
and efficiently converts between image formats, preserving metadata contained
in image files. In particular, if images are stored in `JPEG 2000`_ format,
Sipi can convert them on the fly to formats that are commonly used on the
Internet. Sipi offers a flexible framework for specifying authentication and
authorization logic in Lua_ scripts, and supports restricted access to images,
either by reducing image dimensions or by adding watermarks. It can easily be
integrated with Knora_.

Sipi is `free software`_, released under the `GNU Affero General Public License`_.
It is written in C++ and runs on Linux (including Debian_, Ubuntu_, and CentOS_) and
Mac OS X.

Freely distributable binary releases will be available soon.

.. _IIIF: http://iiif.io/
.. _JPEG 2000: https://jpeg.org/jpeg2000/
.. _Lua: https://www.lua.org/
.. _Digital Humanities Lab: http://www.dhlab.unibas.ch
.. _University of Basel: https://www.unibas.ch/en.html
.. _Knora: http://www.knora.org/
.. _free software: http://www.gnu.org/philosophy/free-sw.en.html
.. _GNU Affero General Public License: http://www.gnu.org/licenses/agpl-3.0.en.html
.. _Debian: https://www.debian.org/
.. _Ubuntu: https://www.ubuntu.com/
.. _CentOS: https://www.centos.org/
