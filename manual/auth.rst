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

################################
Authentication and Authorization
################################

In the config file, ``initscript`` contains the path of a Lua-file that
defines a function called ``pre_flight``. The function takes the
parameters ``prefix``, ``identifier`` and, ``cookie``, and is called
whenever an image is requested from the server.

The possible return values of the ``pre_flight`` function are as follows.
Note that Lua function's return value may consist of more than one element
(see `Multiple Results`_):

- Grant full permissions to access the file identified by ``filepath``: ``return 'allow', filepath``
- Grant restricted access to the file identified by ``filepath``, in one of the following ways:
    - Reduce the image dimensions, e.g. to the default thumbnail dimensions: ``return 'restrict:size=' .. "config.thumb_size", filepath``
    - Render the image with a watermark: ``return restrict:watermark=<path-to-watermark>, filepath``
- Deny access to the requested file: ``return 'deny'``

In the ``pre_flight`` function, permission checking can be implemented.
When Sipi is used with Knora_, the ``pre_flight`` function asks
Knora about the user's permissions on the image
(see ``sipi.init-knora.lua``). The scripts ``Knora_login.lua`` and
``Knora_logout.lua`` handle the setting and unsetting of a cookie
containing the Knora session ID.

For more details on the functionality available to Lua scripts running
in Sipi, see :ref:`lua`.

.. _Multiple Results: http://www.lua.org/pil/5.1.html
.. _Knora: http://www.knora.org/
