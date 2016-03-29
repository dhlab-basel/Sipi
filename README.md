# README #

SIPI (Simple Image Presentation Interface) is a IIIFv2 image server written in C++ that uses several open source libraries for metadata handling. It runs on Unix, Linux (Ubuntu, CentOS) and unix-like systems, including OS X 10.11 El Capitan). Compiling it for windows might be possible, but it is up to you - good luck!

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
For more information, please have a look at the comments in the config file.

If you intend to use Sipi with Knora, use `sipi.knora-config.lua` (in that case, make sure that you install the required packages for lua, see below).

Add the directory `cache` in the main directory.

In the main directory, call:

```bash
build/sipi -config sipi.config.lua
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

To convert an image to a specified format, an HTTP request can be sent to a convert route as defined in the sipi config file (see `sipi.knora-config.lua`).

For use with Knora, you find test scripts and test images in the folder `test-server`.

## Create Documentation

Documentation is created using `doxygen`. First, adapt `Doxyfile.in`:

- OUTPUT_DIRECTORY = path_to_sipi/doc
- INPUT = path_to_sipi path_to_sipi/shttps

Then run doxygen in the sipi main directory: `doxygen Doxygen.in`. You will the find the documentation in the specified output directory. To create a pdf, go to directory `doc/latex` and run `make`. This will create a file called `refman.pdf`.

## Using Lua scripts

Within Sipi, Lua is used to write custom routes. Sipi provides the Lua-interpreter and pack manager `luarocks` as executables. **Sipi does not use your system's Lua interpreter or package manager.**

### Installing Lua modules

To install Lua modules that can be used in Lua scripts, use `local/bin/luarocks`. Please take care that the location where the modules get stored are in the lua package path: `local/bin/lurocks path`. The Lua paths will be used by the Lua interpreter when loading modules in a script with `require`, see: <http://leafo.net/guides/customizing-the-luarocks-tree.html>.

For example, using `local/bin/luarocks install --local package` the package will be installed in `~/.luarocks/`. To include this path in the Lua's interpreter package search path, you can use an environment variable. Running `local/bin/luarocks path` outputs the code you can use to do so. Alternatively, you can build the package path at the beginning of a Lua file by setting `package.path` and `package.cpath` (see: <http://leafo.net/guides/customizing-the-luarocks-tree.html#the-install-locations/using-a-custom-directory/quick-guide/running-scripts-with-packages>).

To make this easier in the future, we will provide a file that is automatically passed to the Lua interpreter, settings the paths correctly (using `lua -l set_paths`).

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
