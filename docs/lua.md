# Customizing Sipi with Lua Scripts

Within Sipi, Lua is used to perform authentication and authorization for
IIIF image requests, and to write custom routes.

Sipi provides the Lua interpreter the [LuaRocks](https://luarocks.org/)
package manager. Sipi does not use the system's Lua interpreter or
package manager.

The Lua interpreter in Sipi runs in a multithreaded environment: each
request runs in its own thread and has its own Lua interpreter.
Therefore, only Lua packages that are known to be thread-safe may be
used.

## Custom Routes

Custom routes can be defined in Sipi's configuration file using the
`routes` configuration variable. For example:

    routes = {
        {
            method = 'GET',
            route = '/status',
            script = 'get_repository_status.lua'
        },
        {
            method = 'POST',
            route = '/make_thumbnail',
            script = 'make_image_thumbnail.lua'
        }
    }

Sipi looks for these scripts in the directory specified by `scriptdir`
in its configuration file. The first route that matches the beginning of
the requested URL path will be used.

## Authentication and Authorization

In Sipi's config file, `initscript` contains the path of a Lua script
that defines a function called `pre_flight`. The function takes the
parameters `prefix`, `identifier` and, `cookie`, and is called whenever
an image is requested.

The possible return values of the `pre_flight` function are as follows.
Note that Lua function's return value may consist of more than one
element (see [Multiple Results](http://www.lua.org/pil/5.1.html)):

-   Grant full permissions to access the file identified by `filepath`:
    `return 'allow', filepath`
-   

    Grant restricted access to the file identified by `filepath`, in one of the following ways:

    :   -   Reduce the image dimensions, e.g. to the default thumbnail
            dimensions:
            `return 'restrict:size=' .. "config.thumb_size", filepath`
        -   Render the image with a watermark:
            `return restrict:watermark=<path-to-watermark>, filepath`

-   Deny access to the requested file: `return 'deny'`

In the `pre_flight` function, permission checking can be implemented.
When Sipi is used with [Knora](http://www.knora.org/), the `pre_flight`
function asks Knora about the user's permissions on the image (see
`sipi.init-knora.lua`). The scripts `Knora_login.lua` and
`Knora_logout.lua` handle the setting and unsetting of a cookie
containing the Knora session ID.

## File uploads to SIPI

Using Lua it is possible to create an upload function for image files.
See the scripts `upload.elua` and `do-upload.elua` in the server
directory, or `upload.lua` in the scripts directory.

## Sipi Functions Available to Lua Scripts

Sipi provides the following functions that can be called from Lua
scripts. Each function returns two values. The first value is `true` if
the operation succeeded, `false` otherwise. If the operation succeeded,
the second value is the result of the operation, otherwise it is an
error message.

### server.setBuffer

    success, errmsg = server.setBuffer([bufsize][,incsize])

Activates the the connection buffer. Optionally the buffer size and
increment size can be given. Returns `true, nil` on success or
`false, errormsg` on failure.

### server.fs.ftype

    success, filetype = server.fs.ftype(filepath)

Checks the filetype of a given filepath. Returns either `true, filetype`
(one of `"FILE"`, `"DIRECTORY"`, `"CHARDEV"`, `"BLOCKDEV"`, `"LINK"`,
`"SOCKET"` or `"UNKNOWN"`) or `false, errormsg`.

### server.fs.modtime

    success, modtime = server.fs.modtime(filepath)

Retrieves the last modification date of a file in seconds since epoch
UTC. Returns either `true`, `modtime` or `false`, `errormsg`.

### server.fs.is\_readable

    success, readable = server.fs.is_readable(filepath)

Checks if a file is readable. Returns `true, readable` (boolean) on
success or `false, errormsg` on failure.

### server.fs.is\_writeable

    success, writeable = server.fs.is_writeable(filepath)

Checks if a file is writeable. Returns `true, writeable` (boolean) on
success or `false, errormsg` on failure.

### server.fs.is\_executable

    success, errormsg = server.fs.is_executable(filepath)

Checks if a file is executable. Returns `true, executable` (boolean) on
success or `false, errormsg` on failure.

### server.fs.exists

    success, exists = server.fs.exists(filepath)

Checks if a file exists. Checks if a file exists. Returns `true, exists`
(boolean) on success or `false, errormsg` on failure.

### server.fs.unlink

    success, errormsg = server.fs.unlink(filename)

Deletes a file from the file system. The file must exist and the user
must have write access. Returns `true, nil` on success or
`false, errormsg` on failure.

### server.fs.mkdir

    success, errormsg = server.fs.mkdir(dirname, [tonumber('0755', 8)])

Creates a new directory, optionally with the specified permissions.
Returns `true, nil` on success or `false, errormsg` on failure.

### server.fs.rmdir

    success, errormsg = server.fs.rmdir(dirname)

Deletes a directory. Returns `true, nil` on success or `false, errormsg`
on failure.

### server.fs.getcwd

    success, curdir = server.fs.getcwd()

Gets the current working directory. Returns `true, current_dir` on
success or `false, errormsg` on failure.

### server.fs.readdir

    success, filenames = server.fs.readdir(dirname)

Gets the names of the files in a directory, not including `.` and `..`.
Returns `true, table` on success or `false, errormsg` on failure.

### server.fs.chdir

    success, oldir = server.fs.chdir(newdir)

Change working directory. Returns `true, olddir` on success or
`false, errormsg` on failure.

### server.fs.copyFile

    success, errormsg = server.fs.copyFile(source, destination)

Copies a file from source to destination. Returns `true, nil`on success
or `false, errormsg` on failure.

### server.fs.moveFile

    success, errormsg = server.fs.moveFile(from, to)

Moves a file. The move connot cross filesystem boundaries! `true, nil`on
success or `false, errormsg` on failure.

### server.uuid

    success, uuid = server.uuid()

Generates a random UUID version 4 identifier in canonical form, as
described in [RFC 4122](https://tools.ietf.org/html/rfc4122). Returns
`true, uuid` on success or `false, errormsg` on failure.

### server.uuid62

    success, uuid62 = server.uuid62()

Generates a Base62-encoded UUID. Returns `true, uuid62` on success or
`false, errormsg` on failure.

### server.uuid\_to\_base62

    success, uuid62 = server.uuid_to_base62(uuid)

Converts a canonical UUID string to a Base62-encoded UUID. Returns
`true, uuid62` on success or `false, errormsg` on failure.

### server.base62\_to\_uuid

    success, uuid = server.base62_to_uuid(uuid62)

Converts a Base62-encoded UUID to canonical form. Returns `true, uuid`
on success or `false, errormsg` on failure.

### server.print

    success, errormsg = server.print(values)

Prints variables and/or strings to the HTTP connection. Returns
`true, nil` on success or `false, errormsg` on failure.

### server.http

    success, result = server.http(method, "http://server.domain[:port]/path/file" [, header] [, timeout])

Performs an HTTP request. Parameters:

-   `method`: The HTTP request method. Currently must be `"GET"`.
-   `url`: The HTTP URL.
-   `header`: An optional table of key-value pairs representing HTTP
    request headers.
-   `timeout`: An optional number of milliseconds until the connection
    times out.

Authentication is not yet supported.

The result is a table:

    result = {
        status_code = value -- HTTP status code returned
        erromsg = "error description" -- only if success is false
        header = {
            name = value [, name = value, ...]
        },
        certificate = { -- only if HTTPS connection
            subject = value,
            issuer = value
        },
        body = data,
        duration = milliseconds
    }

Example:

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

### server.table\_to\_json

::

:   success, jsonstr = server.table\_to\_json(table)

Converts a (nested) Lua table to a JSON string. Returns `true, jsonstr`
on success or `false, errormsg` on failure.

### server.json\_to\_table

    success, table = server.json_to_table(jsonstr)

Converts a JSON string to a (nested) Lua table. Returns `true, table` on
success or `false, errormsg` on failure.

### server.sendHeader

    success, errormsg = server.sendHeader(key, value)

Sets an HTTP response header. Returns `true, nil` on success or
`false, errormsg` on failure.

### server.sendCookie

    success, errormsg = server.sendCookie(key, value [, options-table])

Sets a cookie in the HTTP response. Returns `true, nil` on success or
`false, errormsg` on failure. The optional `options-table` is a Lua
table containing the following keys:

-   `path`
-   `domain`
-   `expires` (value in seconds)
-   `secure` (boolean)
-   `http_only` (boolean)

### server.sendStatus

    server.sendStatus(code)

Sends an HTTP status code. This function is always successful and
returns nothing.

### server.generate\_jwt

    success, token = server.generate_jwt(table)

Generates a [JSON Web Token](https://jwt.io/) (JWT) with the table as
payload. Returns `true, token` on success or `false, errormsg` on
failure. The table contains the JWT claims as follows. (The type
`IntDate` is a number of seconds since 1970-01-01T0:0:0Z):

-   `iss` (string =&gt; StringOrURI) OPT: principal that issued the JWT.
-   `exp` (number =&gt; IntDate) OPT: expiration time on or after which
    the token MUST NOT be accepted for processing.
-   `nbf` (number =&gt; IntDate) OPT: identifies the time before which
    the token MUST NOT be accepted for processing.
-   `iat` (number =&gt; IntDate) OPT: identifies the time at which the
    JWT was issued.
-   `aud` (string =&gt; StringOrURI) OPT: identifies the audience that
    the JWT is intended for. The audience value is a string, typically
    the base address of the resource being accessed, such as
    `https://contoso.com`.
-   `prn` (string =&gt; StringOrURI) OPT: identifies the subject of
    the JWT.
-   `jti` (string =&gt; String) OPT: provides a unique identifier for
    the JWT.

### server.decode\_jwt

    success, table = server.decode_jwt(token)

Decodes a [JSON Web Token](https://jwt.io/) (JWT) and returns its
content as table. Returns `true, table` on success or `false, errormsg`
on failure.

### server.file\_mimetype

    success, table = server.file_mimetype(path)
    success, table = server.file_mimetype(index)

Determines the mimetype of a file. The first form is used if the file
path is known. The second form can be used for uploads by passing the
file index. It returns `true, table` on success or `false, errormsg` on
failure. The table has 2 members: - `mimetype` - `charset`

### server.file\_mimeconsistency

    success, is_consistent = server.file_mimeconsistency(path)
    success, is_consistent = server.file_mimeconsistency(index)
    
Checks if the file extension and the mimetype determined by the magic
of the file is consistent. The first form requires a path (including the
filename with extension), the second can be used for checking uploads by 
passing the file index. It returns `true, is_consistent` on success or
`false, errormsg` in case of an error. `is_consistent` is true if the
mimetype corresponds to the file extension.

### server.requireAuth

    success, table = server.requireAuth()

Gets HTTP authentication data. Returns `true, table` on success or
`false, errormsg` on failure. The result is a table:

    {
        status = string -- "BASIC" | "BEARER" | "NOAUTH" (no authorization header) | "ERROR"
        username = string -- only if status = "BASIC"
        password = string -- only if status = "BASIC"
        token = string -- only if status = "BEARER"
        message = string -- only if status = "ERROR"
    }

Example:

    success, auth = server.requireAuth()
    if not success then
        server.sendStatus(501)
        server.print("Error in getting authentication scheme!")
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

### server.copyTmpfile

    success, errormsg = server.copyTmpfile(from, to)

Sipi saves each uploaded file in a temporary location (given by the
config variable `tmpdir`) and deletes it after the request has been
served. This function is used to copy the file to another location where
it can be retrieved later. Returns `true, nil` on success or
`false, errormsg` on failure.

Parameters:
- `from`:    an index (integer value) of array server.uploads.
- `target`:  an absolute path

### server.systime

    systime = server.systime()

Returns the current system time on the server in seconds since epoch.

### server.log

    server.log(message, loglevel)

Writes a message to
[syslog](http://man7.org/linux/man-pages/man3/syslog.3.html). Severity
levels are:

-   `server.loglevel.LOG_EMERG`
-   `server.loglevel.LOG_ALERT`
-   `server.loglevel.LOG_CRIT`
-   `server.loglevel.LOG_ERR`
-   `server.loglevel.LOG_WARNING`
-   `server.loglevel.LOG_NOTICE`
-   `server.loglevel.LOG_INFO`
-   `server.loglevel.LOG_DEBUG`

## Sipi Variables Available to Lua Scripts

-   `config.hostname`: Hostname where SIPI runs on
-   `config.port`: Portnumber where SIPI communicates (non SSL)
-   `config.sslport`: Portnumber for SSL connections of SIPI
-   `config.imgroot`: Root directory for IIIF-served images
-   `config.docroot`: Root directory for WEB-Server
-   `config.max_temp_file_age`: maximum age of temporary files
-   `config.prefix_as_path`: `true`if the prefix should be used as
    internal path image directories
-   `config.init_script`: Path to initialization script
-   `config.scriptdir`: Path to script directory
-   `config.cache_dir`: Path to cache directory for iIIF served images
-   `config.cache_size`: Maximal size of cache
-   `config.cache_n_files`: Maximal number of files in cache
-   `config.cache_hysteresis`: Amount fo data to be purged if cache
    reaches maximum size
-   `config.keep_alive`: keep alive time
-   `config.thumb_size`: Default thumbnail image size
-   `config.n_threads`: Number of threads SIPI uses
-   `config.max_post_size`: Maximal size of POST data allowed
-   `config.tmpdir`: Temporary directory to store uploads
-   `config.knora_path`: Path to knora REST API (only for SIPI used
    with Knora)
-   `config.knora_port`: Port that the Knora API uses
-   `config.adminuser`: Name of admin user
-   `config.password`: Password of admin user (use with caution)!
-   `server.has_openssl`: `true` if OpenSSL is available.
-   `server.secure`: `true` if the connection was made over HTTPS.
-   `server.host`: the hostname of the Sipi server that was used in
    the request.
-   `server.client_ip`: the IPv4 or IPv6 address of the client
    connecting to Sipi.
-   `server.client_port`: the port number of the client socket.
-   `server.uri`: the URL path used to access Sipi (does not include
    the hostname).
-   `server.header`: a table containing all the HTTP request headers
    (in lowercase).
-   `server.cookies`: a table of the cookies that were sent with
    the request.
-   `server.get`: a table of GET request parameters.
-   `server.post`: a table of POST or PUT request parameters.
-   `server.request`: all request parameters.
-   

    `server.uploads`: an array of upload parameters, one per file. Each one is a table containing:

    :   -   `fieldname`: the name of the form field.
        -   `origname`: the original filename.
        -   `tmpname`: a temporary path to the uploaded file.
        -   `mimetype`: the MIME type of the uploaded file as provided
            by the browser.
        -   `filesize`: the size of uploaded file in bytes.

## Lua helper functions

### helper.filename\_hash

    success, filepath = helper.filename_hash(fileid)

if `subdir_levels` (see configuration file) is &gt; 0, recursive
subdirectories named 'A', 'B',.., 'Z' are used to split the image files
across multiple directories. A simple hash-algorithm is being used. This
function returns a filepath with the subdirectories prepended, e.g
`gaga.jp2` becomes `C/W/gaga.jpg`

Example:

    success, newfilepath = helper.filename_hash(newfilename[imgindex]);
    if not success then
        server.sendStatus(500)
        server.log(gaga, server.loglevel.LOG_ERR)
        return false
    end

    filename = config.imgroot .. '/' .. newfilepath

## Lua image functions

There is an image object implemented which allows to manipulate and
convert images.

### SipiImage.new(filename)

The simple forms are:

    img = SipiImage.new("filename")
    img = SipiImage.new(index)

The first variant opens a file given by "filename", the second variant
opens an uploaded file directly using the integer index to the uploaded
files.

If the index of an uploaded file is passed as an argument, this method
adds additional metadata to the `SipiImage` object that is constructed:
the file's original name, its MIME type, and its SHA256 checksum. When
the `SipiImage` object is then written to another file, this metadata
will be stored in an extra header record.

If a filename is passed, the method does not add this metadata.

The more complex form is as follows:

    img = SipiImage.new("filename", {
            region=<iiif-region-string>,
            size=<iiif-size-string>,
            reduce=<integer>,
            original=origfilename,
            hash="md5"|"sha1"|"sha256"|"sha384"|"sha512"
          })

This creates a new Lua image object and loads the given image into. The
second form allows to indicate a region, the size or a reduce factor and
the original filename. The `hash` parameter indicates that the given
checksum should be calculated out of the pixel values and written into
the header.

### SipiImage.dims()

    success, dims = img.dims()
    if success then
        server.print('nx=', dims.nx, ' ny=', dims.ny)
    end

Returns the dimensions of the image.

### SipiImage.crop(&lt;iiif-region-string&gt;)

    success, errormsg = img.crop(<IIIF-region-string>)

Crops the image to the given rectangular region. The parameter must be a
valid IIIF-region string.

### SipiImage.scale(&lt;iiif-size-string&gt;)

    success, errormsg = img.scale(<iiif-size-string>)

Resizes the image to the given size as iiif-conformant size string.

### SipiImage.rotate(&lt;iiif-rotation-string&gt;)

    success, errormsg = img.rotate(<iiif-rotation-string>)

> Rotates and/or mirrors the image according the given iiif-conformant
> rotation string.

### SipiImage.watermark(&lt;wm-file-path&gt;)

    success, errormsg = img.watermark(<wm-file-path>)

Applies the given watermark file to the image. The watermark file must
be a bitonal TIFF file.

### SipiImage.write(&lt;filepath&gt;)

    success, errormsg = img.write(<filepath>)

    success, errormsg = img.write('HTTP.jpg')

The first version write the image to a file, the second writes the file
to the HTTP connection. The file format is determined by the extension:

-   `jpg` : writes a JPEG file
-   `tif` : writes a TIFF file
-   `png` : writes a png file
-   `jpx` : writes a JPGE2000 file

### SipiImage.send(&lt;format&gt;)

    success, errormsg = img.send(<format>)

Sends the file to the HTTP connection. As format are allowed:

-   `jpg` : writes a JPEG file
-   `tif` : writes a TIFF file
-   `png` : writes a png file
-   `jpx` : writes a JPGE2000 file

## Installing Lua modules

To install Lua modules that can be used in Lua scripts, use
`local/bin/luarocks`. Make sure that the location where the modules are
stored is in the Lua package path, which is printed by
local/bin/lurocks path. The Lua paths will be used by the Lua
interpreter when loading modules in a script with `require` (see [Using
LuaRocks to install packages in the current
directory](http://leafo.net/guides/customizing-the-luarocks-tree.html)).

For example, using `local/bin/luarocks install --local package`, the
package will be installed in `~/.luarocks/`. To include this path in the
Lua's interpreter package search path, you can use an environment
variable. Running `local/bin/luarocks path` outputs the code you can use
to do so. Alternatively, you can build the package path at the beginning
of a Lua file by setting `package.path` and `package.cpath` (see
[Running scripts with
packages](http://leafo.net/guides/customizing-the-luarocks-tree.html#the-install-locations/using-a-custom-directory/quick-guide/running-scripts-with-packages)).

## Using SQLite in Lua Scripts

Sipi supports [SQLite](https://www.sqlite.org/) 3 databases, which can
be accessed from Lua scripts. You should use
[pcall](https://www.lua.org/pil/8.4.html) to handle errors that may be
returned by SQLite.

### Opening an SQLite Database

    db = sqlite('db/test.db', 'RW')

This creates a new opaque database object. The first parameter is the
path to the database file. The second parameter may be `'RO'` for
read-only access, `'RW'` for read-write access, or `'CRW'` for
read-write access. If the database file does not exist, it will be
created using this option.

To destroy the database object and free all resources, you can do this:

``` {.sourceCode .none}
db = ~db
```

However, Lua's garbage collection will destroy the database object and
free all resources when they are no longer used.

### Preparing a Query

    qry = db << 'SELECT * FROM image'

Or, if you want to use a prepared query statement:

    qry = db << 'INSERT INTO image (id, description) VALUES (?,?)'

`qry` will then be a query object containing a prepared query. If the
query object is not needed anymore, it may be destroyed:

``` {.sourceCode .none}
qry = ~qry
```

Query objects should be destroyed explicitly if not needed any longer.

### Executing a Query

    row = qry()
    while (row) do
        print(row[0], ' -> ', row[1])
        row = qry()
    end

Or with a prepared statement:

    qry('SGV_1960_00315', 'This is an image of a steam engine...')

The second way is used for prepared queries that contain parameters.
