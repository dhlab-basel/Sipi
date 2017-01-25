#!/bin/bash
#
# apply patch for buggy cmake in debian 8.* (cmake-3.0)
#
patch /usr/share/cmake-3.0/Modules/FindOpenSSL.cmake < patches/cmake-openssl.patch
