--
-- Copyright © 2016 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
-- Ivan Subotic, Tobias Schweizer, André Kilchenmann, and André Fatton.
-- This file is part of Sipi.
-- Sipi is free software: you can redistribute it and/or modify
-- it under the terms of the GNU Affero General Public License as published
-- by the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
-- Sipi is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
-- Additional permission under GNU AGPL version 3 section 7:
-- If you modify this Program, or any covered work, by linking or combining
-- it with Kakadu (or a modified version of that library) or Adobe ICC Color
-- Profiles (or a modified version of that library) or both, containing parts
-- covered by the terms of the Kakadu Software Licence or Adobe Software Licence,
-- or both, the licensors of this Program grant you additional permission
-- to convey the resulting work.
-- You should have received a copy of the GNU Affero General Public
-- License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.
--
sipi = {
    --
    -- The user under which the Sipi server should run. Use this only if Sipi should setuid to a particular user after
    -- starting. Otherwise, leave this commented out. If this setting is used, Sipi must be started as root.
    --
    -- userid = '_www',

    --
    -- Indicate the hostname (DNS-name), under which the SIPI server is being adressed
    --
    hostname = 'localhost',
    
    --
    -- port number the server is listening to. If SIPI is running on a dedicated system, this should
    -- be set to 80
    --
    port = 1024,

    --
    -- If compiled with SSL support, the port the server is listening for secure connections.
    -- If SIPI is running on a dedicated system, this should be set to 443
    --
    ssl_port = 1025,

    --
    -- Number of threads to use
    --
    nthreads = 8,

    --
    -- Number of seconds a connection (socket) remains open at maximum ("keep-alive")
    --
    keep_alive = 5,

    --
    -- Maximal size of a post request.
    --
    max_post_size = '300M',

    --
    -- indicates the path to the root of the image directory. Depending on the settings of the variable
    -- "prefix_as_path" the images are search at <imgroot>/<prefix>/<imageid> (prefix_as_path = TRUE)
    -- or <imgroot>/<imageid> (prefix_as_path = true). Please note that "prefix" and "imageid" are
    -- expected to be urlencoded. Both will be decoded. That is, "/" will be recognized and expanded
    -- in the final path the image file.
    --
    -- To use Sipi's test data, use the following imgroot, and set prefix_as_path to true below:
    -- imgroot = './test/_test_data/images',
    --
    imgroot = './images',

    --
    -- If true, the IIIF prefix is used to build the path to the image files.
    --
    prefix_as_path = false,

    --
    -- In order not to accumulate to many files into one diretory (which slows down file
    -- access considerabely), the images are stored in recursive subdirectories 'A'-'Z'.
    -- If subdir_levels is equal 0, no subdirectories are used. The maximum is 6.
    -- The recommandeation is that on average there should not me more than a few
    -- thousand files in a unix directory (your mileage may vay depending on the
    -- file system used).
    --
    subdir_levels = 1,

    --
    -- Lua script which is executed on initialization of the Lua interpreter
    --
    initscript = './config/sipi.init.lua',

    --
    -- path to the caching directory
    --
    cachedir = './cache',

    --
    -- maximal size of the cache
    -- The cache will be purged if either the maximal size or maximal number
    -- of files is reached
    --
    cachesize = '200M',

    --
    -- maximal number of files to be cached
    -- The cache will be purged if either the maximal size or maximal number
    -- of files is reached
    --
    cache_nfiles = 250,

    --
    -- if the cache becomes full, the given percentage of file space is marked for reuse
    --
    cache_hysteresis = 0.15,

    --
    -- Path to the directory where the scripts for the routes defined below are to be found
    --
    scriptdir = './scripts',

    ---
    --- Size of the thumbnails (to be used within Lua)
    ---
    thumb_size = '!128,128',

    --
    -- Path to the temporary directory
    --
    tmpdir = '/tmp',

    --
    -- If compiled with SSL support, the path to the certificate (must be .pem file)
    -- The follow commands can be used to generate a self-signed certificate
    -- # openssl genrsa -out key.pem 2048
    -- # openssl req -new -key key.pem -out csr.pem
    -- #openssl req -x509 -days 365 -key key.pem -in csr.pem -out certificate.pem
    --
    ssl_certificate = './certificate/certificate.pem',

    --
    -- If compiled with SSL support, the path to the key file (see above to create)
    --
    ssl_key = './certificate/key.pem',


    --
    -- The secret for generating JWT's (JSON Web Tokens) (exactely 42 characters)
    --
    jwt_secret = 'UP 4888, nice 4-8-4 steam engine',
    --            12345678901234567890123456789012

    --
    -- Name of the logfile (a ".txt" is added...) !!! Currently not used, since logging
    -- is based on syslog !!!!
    --
    logfile = "sipi.log",

    --
    -- loglevel, one of "EMERGENCY", "ALERT", "CRITICAL", "ERROR", "WARNING", "NOTICE", "INFORMATIONAL", "DEBUG"
    --
    loglevel = "WARNING",
}

admin = {
    --
    -- username of admin user
    --
    user = 'admin',

    --
    -- Administration password
    --
    password = 'Sipi-Admin'
}

fileserver = {
    docroot = './server',
    docroute = '/server'
}

--
-- here we define routes that are handled by lua scripts. A route is a defined url:
-- http://<server-DNS>/<route>
-- executes the given script defined below
--
routes = {
    {
        method = 'DELETE',
        route = '/api/cache',
        script = 'cache.lua'
    },
    {
        method = 'GET',
        route = '/api/cache',
        script = 'cache.lua'
    },
    {
        method = 'GET',
        route = '/api/exit',
        script = 'exit.lua'
    },
    {
        method = 'GET',
        route = '/luaexe/test1',
        script = 'test1.lua'
    },
    {
        method = 'GET',
        route = '/luaexe/test2',
        script = 'test2.lua'
    }

}
