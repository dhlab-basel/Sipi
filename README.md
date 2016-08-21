[![Build Status](https://travis-ci.org/dhlab-basel/Sipi.svg?branch=develop)](https://travis-ci.org/dhlab-basel/Sipi)

# Overview #

SIPI (Simple Image Presentation Interface) is a IIIFv2 image server written in C++ that uses several open source libraries for metadata handling. It runs on Unix, Linux (Ubuntu, CentOS) and unix-like systems, including OS X 10.11 El Capitan). Compiling it for windows might be possible, but it is up to you - good luck!

SIPI is developed by the [Digital Humanities Lab](http://www.dhlab.unibas.ch) at the [University of Basel](https://www.unibas.ch/en.html).

SIPI is [free software](http://www.gnu.org/philosophy/free-sw.en.html), released under the [GNU Affero General Public License](http://www.gnu.org/licenses/agpl-3.0.en.html).
SIPI uses the kakadu-library for JPEG2000 support. Kakadu ( http://kakadusoftware.com/ ) is a commercial high-performance library which implements the full JPEG2000 stack. Kakadu must be licensed and downloaded separately by the user. We do not provide the Kakadu library.
The current version of SIPI requires version "v7_8-01382N" of kakadu. The zip-File must be copied into the "vendor" subdirectory with the name "v7_8-01382N.zip".

SIPI uses the Adobe ICC Color profiles which are automatically downloaded by the cmake process into a file called "icc.zip". The
user is responsible for reading and agreeing with the license conditions of Adobe as written in the provided file "Color Profile EULA.pdf"!

The build process relies on cmake.

## Prerequisites ##

### Secure HTTP (https) ###

SIPI supports secure connections (SSL). However, OpenSLL must be installed on the computer. On Linux,
you just have to install the openssl RPMs (including the development version), on OS X use brew.

** The OpenSSL libraries and includes are _not_ downloaded by cmake! **

Cmake checks if OpenSSL is installed and compiles the support for it automatically.
In order to use SIPI with secure connections, You need to install a certificate (see the config file
example "config/sipi.config.lua" for instructions.

### General
- a working c++11 compiler (gcc >= v4.9 or clang)
- cmake > 2.8.0 (for Mac, see below)
- internet connection. During the make process a large amount of open source packages are automatically downloaded. These are:
   - zlib-1.2.8
   - xz-5.2.1
   - libjpeg-v9a
   - jbigkit-2.1
   - tiff-4.0.6
   - expat-2.1.0
   - lcms2-2.7
   - exiv2-0.25
   - libpng16
   - log4cpp-1.1.2rc1
   - Adobe ICC Color profile <http://www.adobe.com/support/downloads/iccprofiles/iccprofiles_mac.html>

In the root directory, two additional directories must be created: `build` and `cache`.

### Mac
- xcode command line tools: `xcode-select --install`
- install brew (apt-get like package manager for Mac): <http://brew.sh>
- install cmake: `brew install cmake`
- install doxygen: `brew install doxygen`

### CentOS
- `yum install package gcc-c++`
- `yum install package cmake`
- `yum install package readline-devel`
- `yum install package gettext`
- `yum install package vim-common`
- `yum install package zlib-devel`
- `yum install package doxygen`
- `yum install package unzip`
- `yum install package patch`

### Debian (>= V8.0 jessie)
To compile SIPI on Debian (>= 8), the following packages have to be installed with apt-get:
- `sudo apt-get install g++`
- `sudo apt-get install cmake`
- `sudo apt-get install git`
- `sudo apt-get install gettext`
- `sudo apt-get install libreadline6 libreadline6-dev`
- `sudo apt-get install libssl-dev`
- `sudo apt-get install doxigen`

Then, cmake has to be patched. Unfortunaltely the cmake-version provided by the
debian packages contains a bug and cannot find the OpenSSL libraries and includes. To apply the patch,
go to the Sipi dicrectory and run

`sudo bash debian-cmake-patch.sh`


### Ubuntu (>= V14)
- `sudo apt-get update`
- `sudo apt-get upgrade`
- `sudo apt-get install g++`
- `sudo apt-get install unzip`
- `sudo apt-get install vim-common`
- `sudo apt-get install git`
- `sudo apt-get install cmake`
- `sudo apt-get install libssl-dev`
- `sudo apt-get install libreadline-dev`


### Fedora Linux
- `sudo yum install vim-common`
- `sudo yum install patch`
- `sudo yum install gcc-c++`
- `sudo yum install git`
- `sudo yum install cmake`
- `sudo yum install readline-devel`
- `sudo yum install openssl-devel`

### OpenSUSe
NOTE: not yet ready ready problem with library names...
- `sudo zypper install gcc-c++`
- `zypper install git`
- `zypper install cmake`
- `sudo zypper install zlib-devel`
- `sudo zypper install libexpat-devel`
- `sudo zypper install patch`
- `sudo zypper install readline-devel`
- `sudo zypper install openssl-devel`



### IDE's

#### CLion
If you are using the [CLion](https://www.jetbrains.com/clion/) IDE, put `-j 1` in Preferences ->
Build, Execution, Deployment -> CMake -> Build options, to prevent CMake from building with multiple processes.
Also, note that code inspection in the CLion editor may not work until it has run CMake.


#### Code::Blocks
If you are using the [Code::Blocks](http://www.codeblocks.org/) IDE, you can build a cdb project:

```bash
cd build
cmake .. -G "CodeBlocks - Unix Makefiles"
```

## Running cmake and make

```bash
cd build
cmake ..
make
```

## Running SIPI-Server

Adapt the config file `sipi.config.lua` (port number and root dir for images `imgroot`).
For more information, please have a look at the comments in the config file.

If you intend to use Sipi with Knora, use `sipi.knora-config.lua` (in that case, make sure that you install the required packages for lua, see below).

Add the directory `cache` in the main directory.

In the main directory, call:

```bash
build/sipi -config config/sipi.config.lua
```

All operations are written to the log file `sipi.log.file`.

## Serving an Image

### Accessing an Image

After SIPI-Server has been started, images can be requested as follows: `http://host:portnumber/prefix/filename.ext/full/full/0/default.jpg`

The given prefix must exist as a folder in the SIPI `imgroot` (defined in the config file) when `prefix_as_path` is set to `true`.

If SIPI is running under port 1024, `prefix_as_path` is set to `true`,  and the requested image `tmp_6931722432531834801.jpx` exists in `imgroot/images`, the URL looks as follows:

`http://localhost:1024/images/tmp_6931722432531834801.jpx/full/full/0/default.jpg`

The URI complies with this pattern: `{scheme}://{server}{/prefix}/{identifier}/{region}/{size}/{rotation}/{quality}.{format}`

The IIIF URI syntax is specified here: <http://iiif.io/api/image/2.0/#uri-syntax>

### Making Use of the Preflight Function

In the config file, `initscript` contains the path of a Lua-file that defines a function called `pre_flight`. The function takes three parameter `prefix`, `identifier` and, `cookie` and is called whenever an image is requested from the server.

The `pre_flight` function is expected to return one of these three values (please note that a Lua function's return value may consist of more than one element: <http://www.lua.org/pil/5.1.html>):
- `return 'allow', filepath`: grant full permissions to access the file identified by `filepath`
- `return 'restrict:size=' .. "config.thumb_size", filepath` or `return restrict:watermark=<path-to-watermark>, filepath`: grant restricted access to the file identified by `filepath`, either by reducing the dimensions (here: default thumbnail dimensions) or by rendering the image with a given watermark
- `return 'deny'`: deny access to requested file

In the `pre_flight` function, permission checking can be implemented. In the case of using Sipi with Knora, the `pre_flight` function asks Knora about the user's permissions on the image (`sipi.init-knora.lua`). The scripts `Knora_login.lua` and `Knora_logout.lua` handle the setting and unsetting of a cookie containing the Knora session id.

## Converting an Image

To convert an image to a specified format, an HTTP request can be sent to a convert route as defined in the sipi config file (see `config/sipi.knora-config.lua`).

For use with Knora, you find test scripts and test images in the folder `test-server`.

## Create Documentation

Documentation is created using `doxygen`. First, adapt `Doxyfile.in`:

- OUTPUT_DIRECTORY = path_to_sipi/doc
- INPUT = path_to_sipi path_to_sipi/shttps

Then run doxygen in the sipi main directory: `doxygen Doxygen.in`. You will the find the documentation in the specified output directory. To create a pdf, go to directory `doc/latex` and run `make`. This will create a file called `refman.pdf`.

## Using Lua scripts

Within Sipi, Lua is used to write custom routes. Sipi provides the Lua-interpreter and pack manager `luarocks` as executables. **Sipi does not use your system's Lua interpreter or package manager.**

### SIPI functions within Lua

Sipi provides the following functions`and preset variables:

- `server.setBuffer([bufsize][,incsize])` : Activates the the connection buffer. Optionally the buffer size and increment size can be given.
- `server.fs.ftype("path")` : Checks the filetype of a given filepath. Returns either "FILE", "DIRECTORY", "CHARDEV", "BLOCKDEV", "LINK", "SOCKET" or "UNKNOWN"
- `server.fs.is_readable(filepath)` : Checks if a file is readable. Returns boolean.
- `server.fs.is_writeable(filepath)` : Checks if a file is writeable. Returns boolean.
- `server.fs.is_executable(filepath)` : Checks if a file is executable. Returns boolean.
- `server.fs.exists(filepath)` : Checks if a file exists. Returns boolean.
- `server.fs.unlink(filename)` : Deletes a file from the file system. The file must exist and the user must have write access.
- `server.fs.mkdir(dirname, tonumber('0755', 8)` : Creates a new directory with given permissions.
- `server.fs.mkdir(dirname)` : Creates a new directory.
- `curdir = server.fs.getcwd()` : Gets the current working directory.
- `oldir = server.fs.chdir(newdir)` : Change working directory.
- `uuid = server.uuid()` : Generates a random version 4 uuid string.
- `uuid62 = server.uuid62()` : Generates a base62-uuid string.
- `uuid62 = server.uuid_to_base62(uuid)` : Converts a uuid-string to a base62 uuid.
- `uuid = server.base62_to_uuid(uuid62)` : Converts a base62-uuid to a "normal" uuid.
- `server.print("string"|var1 [,"string|var]...)` : Prints variables and/or strings to the HTTP connection
- `result = server.http(method, "http://server.domain[:port]/path/file" [, header] [, timeout])`: Get's data from a http server. Parameters:
   - `method` : "GET" (only method allowed so far
   - `url` : complete url including optional port, but no authorization yet
   - `header` : optional table with HTTP-header key-value pairs
   - `timeout`: option number of milliseconds until the connect timeouts. The result is a table:
   ```lua
   result = {
      success = true | false
      status_code = value -- HTTP status code returned
      erromsg = "error description" -- only if success is false
      header = {
        name = value [, name = value, ...]
      },
      certificate = { -- only, if HTTPS connection
        subject = value,
        issuer = value
      },
      body = data,
      duration = milliseconds
   }
   ```
   An example of usage:
   ```lua
   result = server.http("GET", "http://www.salsah.org/api/resources/1", 100)
   
   if (result.success) then
       server.print("<table>")
       server.print("<tr><th>Field</th><th>Value</th></tr>")
       for k,v in pairs(server.header) do
           server.print("<tr><td>", k, "</td><td>", v, "</td></tr>")
       end
       server.print("</table><hr/>")
   
       server.print("Duration: ", result.duration, " ms<br/><hr/>")
       server.print("Body:<br/>", result.body)
   else
      server.print("ERROR: ", result.errmsg)
   end
   ```
- `jsonstr = server.table_to_json(table)` : Convert a table to a JSON string.
- `table = server.json_to_table(jsonstr)` : Convert a JSON string to a (nested) Lua table.
- `server.sendHeader(key, value)` : Adds a new HTTP header field.
- `server.requireAuth()` : Gets Basic HTTP authentification data. The result is a table:
  ```lua`
  {
    status = "BASIC" | "BEARER" | "NOAUTH" | "ERROR", -- NOAUTH means no authorization header
    username = string, -- only if status = "BASIC"
    password = string, -- only if status = "BASIC"
    token = string, -- only if status = "BEARER"
    message = string -- only if status = "ERROR"
  }
  ```
  Usage is as follows (example):
  ```lua
  auth = server.requireAuth()
  
  if auth.status == 'BASIC' then
     --
     -- everything OK, let's create the token for further calls and ad it to a cookie
     --
     if auth.username == config.adminuser and auth.password == config.password then
        tokendata = {
           iss = "sipi.unibas.ch",
           aud = "knora.org",
           user = auth.username
        }
        token = server.generate_jwt(tokendata)
        server.sendCookie('sipi', token, {path = '/', expires = 3600})
     else
        server.sendStatus(401)
        server.sendHeader('WWW-Authenticate', 'Basic realm="SIPI"')
        server.print("Wrong credentials!")
        return -1
     end
  elseif auth.status == 'BEARER' then
     jwt = server.decode_jwt(auth.token)
     if (jwt.iss ~= 'sipi.unibas.ch') or (jwt.aud ~= 'knora.org') or (jwt.user ~= config.adminuser) then
        server.sendStatus(401)
        server.sendHeader('WWW-Authenticate', 'Basic realm="SIPI"')
        return -1
     end
  elseif auth.status == 'NOAUTH' then
     server.setBuffer()
     server.sendStatus(401);
     server.sendHeader('WWW-Authenticate', 'Basic realm="SIPI"')
     return -1
  else
     server.status(401)
     server.sendHeader('WWW-Authenticate', 'Basic realm="SIPI"')
     return -1
  end

  ```
- `server.copyTmpfile()` : shttp saves uploaded files in a temporary location (given by the config variable "tmpdir") and deletes it after the request has been served. This function is used to copy the file to another location where it can be used/retrieved by shttps/sipi.
- `server.has_openssl` : True if openssl is available
- `server.secure` : True, if we are an a secure https connection
- `server.host` : The hostname of the SIPI server that was used in the request.
- `server.client_ip` : IP-Address of the client connecting to SIPI (IP4 or IP6).
- `server.client_port`: Portnumber of client socket.
- `server.uri` : The URL used to access SIPI (exclusive the hostname/dns).
- `server.header` : Table with all HTTP header files. Please note that the names are all lowercase!
- `server.cookies` : Table of cookies.
- `server.get` : Table of GET parameters.
- `server.post` : Table of POST parameter.
- `server.uploads`: Àrray of upload params, for each file a table with:
   - `fieldname` : Name of form-field.
   - `origname` : Original file name.
   - `tmpname` : Temporary path to uploaded file.
   - `mimetype` : MIME-type of uploaded file as provided by the browser.
   - `filesize` : Size of uploaded file as bytes.
- `server.request` : Merge of GET and POST parameters


### Installing Lua modules

To install Lua modules that can be used in Lua scripts, use `local/bin/luarocks`. Please take care that the location where the modules get stored are in the lua package path: `local/bin/lurocks path`. The Lua paths will be used by the Lua interpreter when loading modules in a script with `require`, see: <http://leafo.net/guides/customizing-the-luarocks-tree.html>.

For example, using `local/bin/luarocks install --local package` the package will be installed in `~/.luarocks/`. To include this path in the Lua's interpreter package search path, you can use an environment variable. Running `local/bin/luarocks path` outputs the code you can use to do so. Alternatively, you can build the package path at the beginning of a Lua file by setting `package.path` and `package.cpath` (see: <http://leafo.net/guides/customizing-the-luarocks-tree.html#the-install-locations/using-a-custom-directory/quick-guide/running-scripts-with-packages>).

## Starting Sipi from the GNU Debugger GDB

From the Sipi root dir, start sipi like this:

```bash
gdb build/sipi

(gdb) run -config config/sipi.config.lua

```

some useful command to debug from gdb are: 

```bash

bt (backtrace)  gives a call stack.

frame <args> gives information about a specific frame from the stack.

info locals gives you information about any local variables on the stack.

```


## Commit Message Schema

When writing commit messages, we stick to this schema:

```
type (scope): subject
body
```

Types:

- feature (new feature for the user)
- fix (bug fix for the user)
- docs (changes to the documentation)
- style (formatting, etc; no production code change)
- refactor (refactoring production code, eg. renaming a variable)
- test (adding missing tests, refactoring tests; no production code change)
- build (changes to sbt tasks, CI tasks, deployment tasks, etc.; no production code changes)
- enhancement (residual category)

Example:

```
feature (resources route): add route for resource creation
- add path for multipart request
- adapt handling of resources responder

```
