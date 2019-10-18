v1.x.x Release Notes
====================

v1.2.0 Release Notes
--------------------

See the
[release120](https://github.com/dhlab-basel/Sipi/releases/tag/v1.2.0) on
Github.

### New features:

### Bugfixes:

v1.3.0 Release Notes
--------------------

See the
[release130](https://github.com/dhlab-basel/Sipi/releases/tag/v1.3.0) on
Github.

### New features:

-   Added latest kakadu version v7\_A\_4-01727L.zip
-   support for CIELab for both 8- and 16-bit images
-   try/catch for ICC profiles that are not supported by kakadu. These
    profiles are added to the "essential metadata" in order to be
    reinstated if the JPX is converted back to a TIFF or JPEG.
-   added more unit tests

### Bugfixes:

-   16 Bit PNG images are now teated correctly by byteswapping of the
    data (htons), since PNG uses network byte order which is usually
    noit zthe host byte order on intel processors
-   alpha channels are treated correctly with JPEG2000
-   The parameter names in a multidata/form-data POST request now have
    the double quotes removed

v1.4.0 Release Notes
--------------------

See the
[release140](https://github.com/dhlab-basel/Sipi/releases/tag/v1.4.0) on
Github.

### New features:

-   Added latest kakadu version v7\_A\_4-01727L.zip
-   support for CIELab for both 8- and 16-bit images
-   try/catch for ICC profiles that are not supported by kakadu. These
    profiles are added to the "essential metadata" in order to be
    reinstated if the JPX is converted back to a TIFF or JPEG.
-   added more unit tests

### Bugfixes:

-   16 Bit PNG images are now teated correctly by byteswapping of the
    data (htons), since PNG uses network byte order which is usually
    noit zthe host byte order on intel processors
-   alpha channels are treated correctly with JPEG2000
-   The parameter names in a multidata/form-data POST request now have
    the double quotes removed

v1.5.0-SNAPSHOT Release Notes (not released yet)
------------------------------------------------

See the
[release150](https://github.com/dhlab-basel/Sipi/releases/tag/v1.5.0) on
Github.

### New features:

-   Add path for getting original filename and mimetype
-   Add More functions for lua
-   Add Lua clean\_tempdir() function for cleaning up old
    temporary files.
-   Add server.fs.readdir C++ function for Lua.
-   Add config setting for maximum temp file age.
-   Remove unimplemented log levels TRACE and OFF from docs and configs.
-   Add e2e test of clean\_tempdir().
-   Use clearer names in knora.json response.
-   Log all internal server errors in send\_error.
-   Add more features for Knora integration
-   Improve server.post doc.
-   Add some improvements
-   Don't process a payload in an HTTP DELETE request.

See <https://tools.ietf.org/html/rfc7231#section-4.3.5>:

"A payload within a DELETE request message has no defined semantics;
sending a payload body on a DELETE request might cause some existing
implementations to reject the request."

-   Improve SipiImage.new documentation.
-   Update Kakadu to version v7\_A\_5-01382N

### Bugfixes:

-   Fix unittest
-   Fix to support Essential metadata
-   Fix incorrect log level names in Lua scripts.
-   Fix inconsistencies in log level names (use syslog's
    names everywhere).
-   Fix documentation syntax.

