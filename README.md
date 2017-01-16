[![Build Status](https://travis-ci.org/dhlab-basel/Sipi.svg?branch=develop)](https://travis-ci.org/dhlab-basel/Sipi)

# Overview #

Sipi (Simple Image Presentation Interface) is a IIIFv2 image server written in C++ that uses several open source libraries for metadata handling. It runs on Unix, Linux (Ubuntu, CentOS) and unix-like systems, including OS X 10.11 El Capitan). Compiling it for windows might be possible, but it is up to you - good luck!

Sipi is developed by the [Digital Humanities Lab](http://www.dhlab.unibas.ch) at the [University of Basel](https://www.unibas.ch/en.html).

Sipi is [free software](http://www.gnu.org/philosophy/free-sw.en.html), released under the [GNU Affero General Public License](http://www.gnu.org/licenses/agpl-3.0.en.html).
Sipi uses the kakadu-library for JPEG2000 support. Kakadu ( http://kakadusoftware.com/ ) is a commercial high-performance library which implements the full JPEG2000 stack. Kakadu must be licensed and downloaded separately by the user. We do not provide the Kakadu library.
The current version of Sipi requires version "v7_8-01382N" of kakadu. The zip-File must be copied into the "vendor" subdirectory with the name "v7_8-01382N.zip".

Sipi uses the Adobe ICC Color profiles which are automatically downloaded by the cmake process into a file called "icc.zip". The
user is responsible for reading and agreeing with the license conditions of Adobe as written in the provided file "Color Profile EULA.pdf"!

The build process relies on cmake.

## Prerequisites ##

### Secure HTTP (https) ###

Sipi supports secure connections (SSL). However, OpenSLL must be installed on the computer. On Linux,
you just have to install the openssl RPMs (including the development version), on OS X use brew.

**The OpenSSL libraries and includes are _not_ downloaded by cmake!**

Cmake checks if OpenSSL is installed and compiles the support for it automatically.
In order to use Sipi with secure connections, You need to install a certificate (see the config file
example "config/sipi.config.lua" for instructions.

### General
- a working c++11 compiler (gcc >= v4.8 or clang)
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
- For testing:
  - nginx
  - Python 3.5 or newer
  - libmagic
  - GraphicsMagick

In the root directory, the directory `cache` must be created.

### Mac
- xcode command line tools: `xcode-select --install`
- install brew (apt-get like package manager for Mac): <http://brew.sh>
- install cmake: `brew install cmake`
- install doxygen: `brew install doxygen`
- install openssl: `brew install openssl`
- install libmagic: `brew inwtall libmagic`
- install nginx: `brew install nginx`
- install GraphicsMagick: `brew install graphicsmagick --with-jasper`
- install Python 3: `brew install python3`
- Install Python modules:
  - `pip3 install pytest`
  - `pip3 install requests`
  - `pip3 install psutil`
  - `pip3 install iiif_validator`

### CentOS (V7)
- `sudo yum install gcc-c++`
- `sudo yum install cmake`
- `sudo yum install readline-devel`
- `sudo yum install gettext`
- `sudo yum install vim-common`
- `sudo yum install zlib-devel`
- `sudo yum install doxygen`
- `sudo yum install unzip`
- `sudo yum install patch`
- `sudo yum install openssl-devel`
- `sudo yum install nginx`
- `sudo yum install python35 python35-pip`
- `sudo yum install file-devel`
- `pip3 install pytest`
- `pip3 install requests`
- `pip3 install psutil`
- `pip3 install iiif_validator`

### Debian (>= V8.0 jessie)
To compile Sipi on Debian (>= 8), the following packages have to be installed with apt-get:
- `sudo apt-get install g++`
- `sudo apt-get install cmake`
- `sudo apt-get install git`
- `sudo apt-get install gettext`
- `sudo apt-get install libreadline6 libreadline6-dev`
- `sudo apt-get install libssl-dev`
- `sudo apt-get install doxygen`
- `sudo apt-get install nginx`
- `sudo apt-get install python3`
- `sudo apt-get install libmagic-dev`
- `sudo apt-get install graphicsmagick`
- `pip3 install pytest`
- `pip3 install requests`
- `pip3 install psutil`
- `pip3 install iiif_validator`

Then, cmake has to be patched. Unfortunaltely the cmake-version provided by the
debian packages contains a bug and cannot find the OpenSSL libraries and includes. To apply the patch, go to the Sipi dicrectory and run:

```
$ sudo bash debian-cmake-patch.sh
```

### Ubuntu (>= V14)
- `sudo apt-get update`
- `sudo apt-get upgrade`
- `sudo apt-get install g++`
- `sudo apt-get install unzip`
- `sudo apt-get install vim-common`
- `sudo apt-get install git`
- `sudo apt-get install cmake`
- `sudo apt-get install libssl-dev`
- `sudo apt-get install doxygen`
- `sudo apt-get install libreadline-dev`
- `sudo apt-get install nginx`
- `sudo apt-get install python3`
- `sudo apt-get install libmagic-dev`
- `sudo apt-get install graphicsmagick`
- `pip3 install pytest`
- `pip3 install requests`
- `pip3 install psutil`
- `pip3 install iiif_validator`

### Fedora Linux
- `sudo yum install vim-common`
- `sudo yum install patch`
- `sudo yum install gcc-c++`
- `sudo yum install git`
- `sudo yum install cmake`
- `sudo yum install readline-devel`
- `sudo yum install openssl-devel`
- `sudo yum install nginx`
- `sudo yum install file-devel`
- `sudo yum install python35 python35-pip`
- `sudo yum install GraphicsMagick`
- `pip3 install pytest`
- `pip3 install requests`
- `pip3 install psutil`
- `pip3 install iiif_validator`

### OpenSUSe
NOTE: not yet ready ready problem with library names...
- `sudo zypper install gcc-c++`
- `sudo zypper install git`
- `sudo zypper install cmake`
- `sudo zypper install zlib-devel`
- `sudo zypper install libexpat-devel`
- `sudo zypper install patch`
- `sudo zypper install readline-devel`
- `sudo zypper install openssl-devel`
- `sudo zypper install nginx`
- `sudo zypper install python3`
- `sudo zypper install libmagic-dev`
- `sudo zypper install GraphicsMagick`
- `pip3 install pytest`
- `pip3 install requests`
- `pip3 install psutil`
- `pip3 install iiif_validator`

### IDEs

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

Then after the build, call `make install`.

## Delete previous Build including Dependencies and start over from zero

```bash
cd build
rm -rf * ../lib ../local  ../extsrcs
cmake ..
make
```

## Run the automated tests

```bash
cd build
make check
```

## Running the Sipi server

Adapt the config file `sipi.config.lua`:
- check that the port number is correct and make sure that your operating system's firewall does not block it
- make sure that `imgroot` is set correctly (root dir for images)
- create the directory `cache` in the main directory.

For more information, please have a look at the comments in the config file.

If you intend to use Sipi with Knora, use `sipi.knora-config.lua` (in that case, make sure that you install the required packages for lua, see below).

In the main directory, call:

```bash
local/bin/sipi -config config/sipi.config.lua
```

Logs are written using syslog.

## Serving an Image

### Accessing an Image

After the Sipi server has been started, images can be requested as follows: `http://host:portnumber/prefix/filename.ext/full/full/0/default.jpg`

The given prefix must exist as a folder in the Sipi `imgroot` (defined in the config file) when `prefix_as_path` is set to `true`.

If Sipi is running under port 1024, `prefix_as_path` is set to `true`,  and the requested image `myimage.jpx` exists in `imgroot/images`, the URL looks as follows:

`http://localhost:1024/images/myimage.jpx/full/full/0/default.jpg`

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

## Create Documentation

Documentation is created using `doxygen`. First, adapt `Doxyfile.in`:

- OUTPUT_DIRECTORY = path_to_sipi/doc
- INPUT = path_to_sipi path_to_sipi/shttps

Then run doxygen in the sipi main directory: `doxygen Doxygen.in`. You will the find the documentation in the specified output directory. To create a pdf, go to directory `doc/latex` and run `make`. This will create a file called `refman.pdf`.

## Using Lua scripts

Within Sipi, Lua is used to write custom routes. Sipi provides the Lua-interpreter and pack manager `luarocks` as executables. **Sipi does not use your system's Lua interpreter or package manager.**

**Note:** The Lua interepreter in Sipi runs in a multithreaded environment (each connection runs in its
own thread and has it's own Lua interpreter). Thus only packages that are positivley known to be thread
safe may be used!

### Sipi functions within Lua

Sipi provides the following functions:

- `success, errmsg = server.setBuffer([bufsize][,incsize])`:  
Activates the the connection buffer. Optionally the buffer size and increment size can be given. Returns true, nil on success or false, errormsg on failure.  

- `success, filetype = server.fs.ftype("path")` :  
Checks the filetype of a given filepath. Returns either true, filetype (one of )"FILE", "DIRECTORY", "CHARDEV", "BLOCKDEV", "LINK", "SOCKET" or "UNKNOWN") or false, errormsg  

- `success, readable = server.fs.is_readable(filepath)` :  
Checks if a file is readable. Returns true, readable(boolean) on success or false, errormsg on failure.  

- `success, writeable = server.fs.is_writeable(filepath)` :  
Checks if a file is writeable. Returns true, writeable(boolean) on success or false, errormsg on failure.  

- `success, errormsg = server.fs.is_executable(filepath)` :  
Checks if a file is executable. Returns true, executable(boolean) on success or false, errormsg on failure.  

- `success, exists = server.fs.exists(filepath)` :  
Checks if a file exists. Checks if a file exists. Returns true, exists(boolean) on success or false, errormsg on failure.  

- `success, errormsg = server.fs.unlink(filename)` :  
Deletes a file from the file system. The file must exist and the user must have write access. Returns true, nil on success or false, errormsg on failure.  

- `success, errormsg = server.fs.mkdir(dirname, tonumber('0755', 8)` :  
Creates a new directory with given permissions.  

- `success, errormsg = server.fs.mkdir(dirname)` :  
Creates a new directory. Returns true, nil on success or false, errormsg on failure.  

- `success, errormsg = server.fs.rmdir(dirname)` :  
Deletes a directory. Returns true, nil on success or false, errormsg on failure.  

- `success, curdir = server.fs.getcwd()` :  
Gets the current working directory. Returns true, current_dir on success or false, errormsg on failure.  

- `success, oldir = server.fs.chdir(newdir)` :  
Change working directory. Returns true, olddir on success or false, errormsg on failure.  

- `success, uuid = server.uuid()` :  
Generates a random version 4 uuid string. Returns true, uuid on success or false, errormsg on failure.  

- `success, uuid62 = server.uuid62()` :  
Generates a base62-uuid string. Returns true, uuid62 on success or false, errormsg on failure.  

- `success, uuid62 = server.uuid_to_base62(uuid)` :  
Converts a uuid-string to a base62 uuid. Returns true, uuid62 on success or false, errormsg on failure.  

- `success, uuid = server.base62_to_uuid(uuid62)` :  
Converts a base62-uuid to a "normal" uuid. Returns true, uuid on success or false, errormsg on failure.  

- `sucess, errormsg = server.print("string"|var1 [,"string|var]...)` :  
Prints variables and/or strings to the HTTP connection. Returns true, nil on success or false, errormsg on failure.  

- `success, result = server.http(method, "http://server.domain[:port]/path/file" [, header] [, timeout])`:  
Get's data from a http server. Parameters:
   - `method` : "GET" (only method allowed so far
   - `url` : complete url including optional port, but no authorization yet
   - `header` : optional table with HTTP-header key-value pairs
   - `timeout`: option number of milliseconds until the connect timeouts. The result is a table:
   ```lua
   result = {
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
   success, result = server.http("GET", "http://www.salsah.org/api/resources/1", 100)

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

- `success, jsonstr = server.table_to_json(table)` :  
Convert a table to a JSON string. Returns true, jsonstr on success or false, errormsg on failure.  

- `success, table = server.json_to_table(jsonstr)` :  
Convert a JSON string to a (nested) Lua table. Returns true, table on success or false, errormsg on failure.  

- `success, errormsg = server.sendHeader(key, value)` :  
Adds a new HTTP header field. Returns true, nil on success or false, errormsg on failure.  

- `success, errormsg = server.sendCookie(key, value [, options-table])` :  
Adds a new Cookie. Returns true, nil on success or false, errormsg on failure. options-table as Lua-table:
    - path = "path allowed",
    - domain = "domain allowed",
    - expires = seconds,
    - secure = true | false,
    - http_only = true | false


- `server.sendStatus() `:  
Send HTTP status code. This function is always succesful and returns nothing!  

- `success, token = server.generate_jwt(table)` :  
Generate a Json Web Token (JWT) with the table as payload. Returns true, token on success or false, errormsg on failure. The table contains the jwt-claims. The following claims do have a predefined semantic(IntDate: The number of seconds from 1970-01-01T0:0:0Z):
    - iss (string => StringOrURI) OPT: principal that issued the JWT.
    - exp (number => IntDate) OPT: expiration time on or after which the token. MUST NOT be accepted for processing.
    - nbf  (number => IntDate) OPT: identifies the time before which the token. MUST NOT be accepted for processing.
    - iat (number => IntDate) OPT: identifies the time at which the JWT was issued.
    - aud (string => StringOrURI) OPT: identifies the audience that the JWT is intended for. The audience value is a string -- typically, the base address of the resource being accessed, such as "https://contoso.com"
    - prn (string => StringOrURI) OPT: identifies the subject of the JWT.
    - jti (string => String) OPT: provides a unique identifier for the JWT.


- `success, table = decode_jwt(token)` :  
Decode a jwt token and return the content as table. Returns true, table on success or false, errormsg on failure.  

- `success, table = server.requireAuth()` :  
Gets Basic HTTP authentification data. Returns true, table on success or false, errormsg on failure. The result is a table:
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
  success, auth = server.requireAuth()
  if not success then
    server.sendStatus(501)
    server.print("Error in getting authentification scheme!")
    return -1
  end

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
        success, token = server.generate_jwt(tokendata)
        if not success then
            server.sendStatus(501)
            server.print("Could not generate JWT!")
            return -1
        end
        success, errormsg = server.sendCookie('sipi', token, {path = '/', expires = 3600})
        if not success then
            server.sendStatus(501)
            server.print("Couldn't send cookie with JWT!")
            return -1
        end
     else
        server.sendStatus(401)
        server.sendHeader('WWW-Authenticate', 'Basic realm="Sipi"')
        server.print("Wrong credentials!")
        return -1
     end
  elseif auth.status == 'BEARER' then
     success, jwt = server.decode_jwt(auth.token)
     if not success then
        server.sendStatus(501)
        server.print("Couldn't deocde JWT!")
        return -1
     end
     if (jwt.iss ~= 'sipi.unibas.ch') or (jwt.aud ~= 'knora.org') or (jwt.user ~= config.adminuser) then
        server.sendStatus(401)
        server.sendHeader('WWW-Authenticate', 'Basic realm="Sipi"')
        return -1
     end
  elseif auth.status == 'NOAUTH' then
     server.setBuffer()
     server.sendStatus(401);
     server.sendHeader('WWW-Authenticate', 'Basic realm="Sipi"')
     return -1
  else
     server.status(401)
     server.sendHeader('WWW-Authenticate', 'Basic realm="Sipi"')
     return -1
  end

  ```


- `success, errormsg = server.copyTmpfile()` :  
shttp saves uploaded files in a temporary location (given by the config variable "tmpdir") and deletes it after the request has been served. This function is used to copy the file to another location where it can be used/retrieved by shttps/sipi. Returns true, nil on success or false, errormsg on failure.  
- `server.log(message, loglevel)` :  
Writes a message into the logfile. Loglevels are the ones supported by
[syslog(3)](https://linux.die.net/man/3/syslog):
    - server.loglevel.LOG_EMERG
    - server.loglevel.LOG_ALERT
    - server.loglevel.LOG_CRIT
    - server.loglevel.LOG_ERR
    - server.loglevel.LOG_WARNING
    - server.loglevel.LOG_NOTICE
    - server.loglevel.LOG_INFO
    - server.loglevel.LOG_DEBUG

Sipi provides the following predefined variables:
- `server.has_openssl` : True if openssl is available
- `server.secure` : True, if we are an a secure https connection
- `server.host` : The hostname of the Sipi server that was used in the request.
- `server.client_ip` : IP-Address of the client connecting to Sipi (IP4 or IP6).
- `server.client_port`: Portnumber of client socket.
- `server.uri` : The URL used to access Sipi (exclusive the hostname/dns).
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

## Sqlite3

Sipi supports sqlite3 databases. There is a simple Lua extension built into Sipi.
Please note that the sqlite3 function may produce Lua errors. It is recommended
to use pcall to encapsulate access to the sqlite3 database.

### Opening a sqlite databases

```
db = sqlite('db/test.db', 'RW')
```
This creates a new opaque database object. The first parameter is the path to the database file.
The second parameter may be 'RO' for read-only access, 'RW' for read-write access, or 'CRW'
for read-write access. If the database file does not exist, it will be created using this option.

In order to destroy the database object and free all resources, You can use
```
db = ~db
```
However, also the normal garbage collection of Lua will destroy the database object and
free all resources.

### Preparing a query

```
qry = db << 'SELECT * FROM image'
```
or, if you want to used a prepared query statment:
```
qry = db << 'INSERT INTO image (id, description) VALUES (?,?)'
```
`qry` will then be a query object containing a prepared query. If the query object
is not needed anymore, it may be destroyed by
```
qry = ~qry
```
qry objects should be destroyed explicitely if not needed any longer.

### Executing a query
```
row = qry()
while (row) do
    print(row[0], ' -> ', row[1])
    row = qry()
end
```
or, in order to use a prepared statment:
```
qry('SGV_1960_00315', 'This is an image of a steam engine...')
```
The second way is used for prepared queries that contain parameters.

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

## Writing Tests

The test framework uses [pytest](http://doc.pytest.org/en/latest/). To add tests, add a Python
class in a file whose name begins with `test`, in the `test` directory. The class's methods,
whose names must also begin with `test`, should use the `manager` fixture defined in
`test/conftest.py`, which handles starting and stopping a Sipi server, and provides other functionality
useful in tests. See the existing `test/test_*.py` files for examples.

To facilitate testing client HTTP connections in Lua scripts, the `manager` fixture also starts
and stops an `nginx` instance, which can be used to simulate an authorization server.
For example, the provided `nginx` configuration file, `test/nginx/nginx.conf`, allows `nginx` to
act as a dummy [Knora](http://www.knora.org/) API server for permission checking: its
`/v1/files` route returns a static JSON file that always grants permission to view the requested
file.

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
