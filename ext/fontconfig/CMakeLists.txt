cmake_minimum_required(VERSION 3.14.5)
project(fontconfig_builder C CXX)
include(ExternalProject)

#
# get fontconfig
#

ExternalProject_Add(project_fontconfig
        DEPENDS zlib
        DEPENDS expat
        DEPENDS png
        DEPENDS freetype
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.13.92.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/fontconfig-2.13.1
        CONFIGURE_COMMAND ${COMMON_SRCS}/fontconfig-2.13.1/configure
            --prefix=${COMMON_LOCAL}
            FREETYPE_CFLAGS=-I${CONFIGURE_INCDIR}/freetype2
            FREETYPE_LIBS="-L${CONFIGURE_LIBDIR} -lfreetype -lz -lpng -lbz2 -lm"
            --enable-shared
            --enable-static
        BUILD_COMMAND make
        INSTALL_COMMAND make install
        BUILD_IN_SOURCE 1
        )

ExternalProject_Get_Property(project_fontconfig install_dir)

if(MAKE_SHARED_SIPI)
    add_library(fontconfig SHARED IMPORTED GLOBAL)
    set_property(TARGET fontconfig PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfontconfig${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=ON")
else()
    add_library(fontconfig STATIC IMPORTED GLOBAL)
    set_property(TARGET fontconfig PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfontconfig${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")
endif()

add_dependencies(fontconfig project_fontconfig)
