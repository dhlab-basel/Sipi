cmake_minimum_required(VERSION 2.8.10)
project(fontconfig_builder C CXX)
include(ExternalProject)

#
# get luarocks
#
ExternalProject_Add(project_fontconfig
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.13.1.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/fontconfig
        CONFIGURE_COMMAND ${COMMON_SRCS}/fontconfig/configure
            --enable-static
            --enable-shared
            --prefix=${COMMON_LOCAL}
            --disable-rpath
        BUILD_COMMAND make
        BUILD_IN_SOURCE 1
        )
ExternalProject_Get_Property(project_fontconfig install_dir)

if(MAKE_SHARED_SIPI)
    add_library(fontconfig SHARED IMPORTED GLOBAL)
    set_property(TARGET fontconfig PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfontconfig${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(fontconfig STATIC IMPORTED GLOBAL)
    set_property(TARGET fontconfig PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfontconfig${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(fontconfig project_fontconfig)
