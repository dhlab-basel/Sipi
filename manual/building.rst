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

.. _building:

##############################
Building Sipi from Source Code
##############################

.. contents:: :local:


*************
Prerequisites
*************

To build Sipi from source code, you must have Kakadu_, a JPEG 2000 development
toolkit that is not provided with Sipi and must be licensed separately.
The Kakadu source code archive ``v7_9-01727L.zip`` must be placed in the
``vendor`` subdirectory of the source tree before building Sipi.

Sipi's build process requires CMake_, a C++ compiler that supports the C++11
standard (such as GCC_ or clang_), and several libraries that are readily
available on supported platforms. The test framework requires `Python 3`_ and
nginx_. Instructions for installing these prerequisites are given below.

The build process downloads and builds Sipi's other prerequisites.

Sipi uses the Adobe ICC Color profiles, which are automatically
downloaded by the build process into the file ``icc.zip``. The user
is responsible for reading and agreeing with Adobe's license conditions,
which are specified in the file ``Color Profile EULA.pdf``.


Mac OS X
========

You will need Homebrew_ and at least OSX 10.11.5. Then:

::

    xcode-select --install
    brew install cmake
    brew install doxygen
    brew install openssl
    brew install libmagic
    brew install gettext
    brew install nginx
    sudo chown -R $USER /usr/local/var/log/nginx/
    brew install graphicsmagick --with-jasper
    brew install python3
    pip3 install Sphinx
    pip3 install pytest
    pip3 install requests
    pip3 install psutil
    pip3 install iiif_validator

.. _ubuntu-build:

Ubuntu 16.04
============

::

    sudo apt-get install g++
    sudo apt-get install cmake
    sudo apt-get install libssl-dev
    sudo apt-get install doxygen
    sudo apt-get install libreadline-dev
    sudo apt-get install gettext
    sudo apt-get install nginx
    sudo chown -R $USER /var/log/nginx
    sudo apt-get install python3
    sudo apt-get install python3-pip
    sudo -H pip3 install --upgrade pip
    sudo apt-get install libmagic-dev
    sudo apt-get install graphicsmagick
    sudo -H pip3 install Sphinx
    sudo -H pip3 install pytest
    sudo -H pip3 install requests
    sudo -H pip3 install psutil
    sudo -H pip3 install iiif_validator

Debian 8
========

First, follow the instructions for :ref:`ubuntu-build`.

Then, CMake has to be patched. Unfortunaltely the version of CMake provided
by the Debian packages contains a bug and cannot find the OpenSSL
libraries and includes. To apply the patch, go to the Sipi dicrectory
and run:

::

    sudo ./debian-cmake-patch.sh


CentOS 7
========

This requires GCC_ version 5.3 or greater. You can install it by installing
devtoolset-4_, and adding this to your ``.bash_profile``:

::

    source scl_source enable devtoolset-4

Then:

::

    sudo yum -y install cmake
    sudo yum -y install readline-devel
    sudo yum -y install doxygen
    sudo yum -y install patch
    sudo yum -y install openssl-devel
    sudo yum -y install gettext
    sudo yum -y install nginx
    sudo chown -R $USER /var/log/nginx
    sudo chown -R $USER /var/lib/nginx
    sudo yum -y install file-devel
    sudo yum -y install GraphicsMagick
    sudo yum -y install https://centos7.iuscommunity.org/ius-release.rpm
    sudo yum -y install python35u
    sudo yum -y install python35u-devel
    sudo yum -y install python35u-pip
    sudo pip3.5 install Sphinx
    sudo pip3.5 install pytest
    sudo pip3.5 install requests
    sudo pip3.5 install psutil
    sudo pip3.5 install iiif_validator


*************************
Compiling the Source Code
*************************

Start in the ``build`` subdirectory of the source tree:

::

    cd build

Then compile Sipi:

::

    cmake ..
    make


*************
Running Tests
*************

The tests are currently very incomplete, but you can run them in the ``build`` directory like this:

::

    make check


*******************************************
Making a Directory Tree for Installing Sipi
*******************************************

In ``build``, type this to install Sipi in the ``local`` subdirectory of the source tree:

::

    make install


You can then copy the contents of ``local`` to the desired location.


************************
Generating Documentation
************************

To generate this manual in HTML format, ``cd`` to the ``manual``
subdirectory of the source tree and type:

::

    make html

You will then find the manual under ``manual/_build/html``.

To generate developer documentation about Sipi's C++ internals,
``cd`` to the ``build`` directory and type:

::

    make doc

You will find the developer documentation in HTML format under
``doc/html``. To generate developer documentation in PDF format,
first ensure that you have LaTeX_ installed. Then ``cd``
to ``doc/html/latex`` and type ``make``.

*************
Starting Over
*************

To delete the previous build and start over from scratch, ``cd`` to
the top level of the source tree and type:

::

    rm -rf build/* lib local extsrcs


.. _Kakadu: http://kakadusoftware.com/
.. _CMake: https://cmake.org/
.. _GCC: https://gcc.gnu.org
.. _clang: https://clang.llvm.org/
.. _Python 3: https://www.python.org/
.. _nginx: https://nginx.org/en/
.. _Homebrew: http://brew.sh/
.. _CLion: https://www.jetbrains.com/clion/
.. _`Code::Blocks`: http://www.codeblocks.org/
.. _LaTeX: https://www.latex-project.org/
.. _devtoolset-4: https://www.softwarecollections.org/en/scls/rhscl/devtoolset-4/
