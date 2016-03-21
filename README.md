# README #

SIPI is a IIIFv2 image server written in C++ that uses several open source libraries for metadata handling. It runs on Unix, Linux (Ubuntu, CentOS) and unix-like systems, including OS X 10.11 El Capitan). Compiling it for windows might be possible, but it is up to you - good luck!

SIPI is developed by the [Digital Humanities Lab](http://www.dhlab.unibas.ch) at the [University of Basel](https://www.unibas.ch/en.html).

Knora is [free software](http://www.gnu.org/philosophy/free-sw.en.html), released under the [GNU Affero General Public License](http://www.gnu.org/licenses/agpl-3.0.en.html).
SIPI uses the kakadu-library for JPEG2000 support. Kakadu ( http://kakadusoftware.com/ ) is a commercial high-performance library which implements the full JPEG2000 stack. Kakadu must be licensed and downloaded separately by the user. We do not provide the Kakadu library.

SIPI uses the Adobe ICC Color profile, they must be licensed and downloaded separately by the user.

The build process relies on cmake.

## Prerequisites ##

### General
- a working c++11 compiler (gcc >= v5.3 or clang)
- cmake > 2.8.0 (for Mac, see below)
- java openjdk devel (set environment variable `JAVA_HOME`)
- internet connection. During the make process a large amount of open source packages are downloaded. These are:
   - mariadb-connector-c-2.1.0
   - xz-5.2.1
   - libjpeg-v9a
   - tiff-4.0.6
   - expat-2.1.0
   - lcms2-2.7
   - exiv2-0.25
   - libpng16
   - log4cpp-1.1.2rc1
   - download Adobe ICC Color profile <http://www.adobe.com/support/downloads/iccprofiles/iccprofiles_mac.html> and create two headers file: `xxd -i AdobeRGB1998.icc > AdobeRGB1998_icc.h` and `xxd -i USWebCoatedSWOP.icc > USWebCoatedSWOP_icc.h`

In the root directory, two additional directories must be created: `build` and `cache`.

### Mac
- xcode command line tools: `xcode-select --install`
- install brew (apt-get like package manager for Mac): <http://brew.sh>
- install cmake: `brew install cmake`
- install doxygen: `brew install doxygen`

#### CLion
If you are using the [CLion](https://www.jetbrains.com/clion/) IDE, put `-j 1` in Preferences ->
Build, Execution, Deployment -> CMake -> Build options, to prevent CMake from building with multiple processes.
Also, note that code inspection in the CLion editor may not work until it has run CMake.

### CentOS
- `yum install package gcc-c++`
- `yum install package readline-devel`
- `yum install package zlib-devel`
- `yum install package doxygen`
- `yum install package unzip`
- `yum install package patch`

### Ubuntu
- `sudo apt-get install libreadline-dev`
- `sudo apt-get install log4cpp`
- `gcc >= v5.3` (see below)

#### Code::Blocks
If you are using the [Code::Blocks](http://www.codeblocks.org/) IDE, you can build a cdb project:

```bash
cd build
cmake .. -G "CodeBlocks - Unix Makefiles"
```

#### Install GCC 5.3 and make it the default compiler:

 The assumed current version of gcc is 4.8. Please adapt the version information for the code below if your current gcc version is different.

```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-5 g++-5
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 10
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 20
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 10
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 20
sudo update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 30
sudo update-alternatives --set cc /usr/bin/gcc
sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 30
sudo update-alternatives --set c++ /usr/bin/g++
```

Then run `gcc -v` which should say `gcc version 5.3.0`

## Running cmake and make

```bash
cd build
cmake ..
make
```

## Running SIPI-Server

Adapt the config file `sipi.config.lua` (port number and root dir for images `imgroot`).

Add the directory `cache` in the main directory.

In the main directory, call:

```bash
build/sipi -config sipi.config.lua
```

All operations ate written to the log file `sipi.log.file`.

## Converting an Image

To convert an image to a specified format, a HTTP POST request can be sent to the convert route:

The POST request must contain the following parameters:
- `source`: the path to the image to be converted (e.g., `/tmp/myimage.tiff`)
- `originalMimetype`: the mime type og the file to be converted
- `originalFilename`: the original name of the file to be converted
You find test scripts and test images in the folder `test-server`.

### Testing CORS

The SIPI routes will be also accessed directly by a web browser (SALSAH web interface) and thus needs to support CORS (cross-domain requests).
With the following code, this can be tested for a POST-request to SIPI's conversion handler.

Access <http://www.salsah.org> in your web browser and run the following code in your JavaScript-interpreter:


```javascript
$.ajax({
  method: "POST",
  url: "http://sipihost:1024/convert",
  data: {
    source: "./test_server/images/Chlaus.jpg",
    originalMimetype: "image/jpeg",
    originalFilename: "Chlaus.jpg"
  }
})
```

You should get a JSON response from SIPI. If there is a problem with the CORS support, the browser won't let you do the request.

You can also do a POST multipart request directly sending a binary image. Go to <http://www.salsah.org>, open the form to create a new resource and choose an image to upload (do not click upload). Now you can paste in the following JavaScript-code to send that image to sipi:

```javascript
var formData = new FormData();
// here, we access the form element containing the image
formData.append('file', $('input[type=file]')[0].files[0]);

$.ajax({
        type:'POST',
  url: 'http://http://sipihost:1024/convert',
        data: formData,
        cache: false,
        contentType: false,
        processData: false,
        success:function(data) {
        },
        error: function(data) {
        }
    });
```

If this works, SIPI processes the request and sends you a message back.

## Serving an Image

After SIPI-Server has been started, images can be requested as follows: `http://host:portnumber/folder/filename.ext/full/full/0/default.jpg`

The given folder must exist in the SIPI `imgroot` (`sipi.config.lua`).

If SIPI is running under port 1024 and the requested image `tmp_6931722432531834801.jpx` exists in `imgroot/webapi_tmp`, the URL looks as follows:

`http://localhost:1024/webapi_tmp/tmp_6931722432531834801.jpx/full/full/0/default.jpg`

The URI complies with this pattern: `{scheme}://{server}{/prefix}/{identifier}/{region}/{size}/{rotation}/{quality}.{format}`

The IIIF URI syntax is specified here: <http://iiif.io/api/image/2.0/#uri-syntax>

## Create Documentation

Documentation is created using `doxygen`. First, adapt `Doxyfile.in`:

- OUTPUT_DIRECTORY = path_to_sipi/doc
- INPUT = path_to_sipi path_to_sipi/shttps

Then run doxygen in the sipi main directory: `doxygen Doxygen.in`. You will the find the documentation in the specified output directory. To create a pdf, go to directory `doc/latex` and run `make`. This will create a file called `refman.pdf`.

## Using Lua scripts

Within Sipi, Lua is used to write custom routes. Install the Lua interpreter and the package manager `luarocks` on your system (`brew` or `apt-get`).

To make HTTP-requests from Sipi server (e.g. to ask Knora about file permissions), the module `lua-requests` can be used. It can be installed `luarocks`:

```lua
luarocks install lua-requests
```

`lua-requests` requires `luasec` which may cause problems when installing it because it cannot find openssl header files.

On **Mac**, install openssl using `brew` and create symlinks:

```bash
brew install openssl

# replace YOURVERSION with the current version installed by brew
ln -s /usr/local/Cellar/openssl/$YOURVERSION/include/openssl /usr/local/include/openssl

brew link openssl --force
```

See: <https://github.com/phusion/passenger/issues/1630>

Then install `luasec`:

- Mac: `luarocks install luasec OPENSSL_DIR=/usr/local`
- Linux: ```
sudo luarocks install luasec OPENSSL_LIBDIR=/usr/lib/`gcc -print-multiarch`
``` (see: <https://github.com/Mashape/kong/issues/24>)

Set Lua paths in your `.bashrc` (Linux) or `.bash_profile` (Mac):
```bash
luarocks path >> ~/.bashrc

```

The Lua paths will be used by the Lua interpreter when loading modules in a script with `require`.

See: <http://leafo.net/guides/customizing-the-luarocks-tree.html>

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
