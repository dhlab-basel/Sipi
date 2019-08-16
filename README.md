[![Build Status](https://travis-ci.org/dhlab-basel/Sipi.svg?branch=develop)](https://travis-ci.org/dhlab-basel/Sipi)

# Overview

Sipi is a high-performance media server developed by the [Digital Humanities Lab](http://www.dhlab.unibas.ch) at the
[University of Basel](https://www.unibas.ch/en.html). It is designed to be used by archives,
libraries, and other institutions that need to preserve high-quality images
while making them available online.

Sipi implements the [International Image Interoperability Framework (IIIF)](http://iiif.io/),
and efficiently converts between image formats, preserving metadata contained
in image files. In particular, if images are stored in [JPEG 2000](https://jpeg.org/jpeg2000/) format,
Sipi can convert them on the fly to formats that are commonly used on the
Internet. Sipi offers a flexible framework for specifying authentication and
authorization logic in [Lua](https://www.lua.org/) scripts, and supports restricted access to images,
either by reducing image dimensions or by adding watermarks. It can easily be
integrated with [Knora](http://www.knora.org).

Sipi is [free software](http://www.gnu.org/philosophy/free-sw.en.html),
released under the [GNU Affero General Public License](http://www.gnu.org/licenses/agpl-3.0.en.html).

It is written in C++ and runs on Linux (including Debian, Ubuntu, and CentOS) and
Mac OS X.

Freely distributable binary releases will be available soon.

# Documentation

The manual is online at http://dhlab-basel.github.io/Sipi/.

To build it locally, you will need [Sphinx](http://www.sphinx-doc.org/en/stable/index.html).
In the `manual` subdirectory of the source tree, type:

```
make html
```

You will then find the manual under `manual/_build/html`.

# Build inside Docker using ccache

```bash
$ docker run -v $PWD:/sipi -v ~/ccache:/ccache -e CCACHE_DIR=/ccache dhlabbasel/sipi-base:18.04 /bin/sh -c "cd /sipi/build; cmake .. && make && ctest --verbose"
OR
$ docker run -v $PWD:/sipi -v ~/ccache:/ccache -e CCACHE_DIR=/ccache dhlabbasel/sipi-base:18.04 /bin/sh -c "cd /sipi/build; cmake -G Ninja .. && ninja && ctest --verbose"
```

# Contact Information

Lukas Rosenthaler `<lukas.rosenthaler@unibas.ch>`
