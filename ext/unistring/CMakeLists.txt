cmake_minimum_required(VERSION 3.14.5)
project(unistring_builder C CXX)
include(ExternalProject)

#
# get lcms2 Version 2.8
#
ExternalProject_Add(project_unistring
    INSTALL_DIR ${COMMON_LOCAL}
    URL http://ftp.gnu.org/gnu/libunistring/libunistring-0.9.10.tar.xz
    SOURCE_DIR ${COMMON_SRCS}/unistring
    CONFIGURE_COMMAND ${COMMON_SRCS}/unistring/configure --enable-static --enable-shared --prefix=${COMMON_LOCAL}  --libdir=${CONFIGURE_LIBDIR}
    BUILD_COMMAND make
    BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_unistring install_dir)
if(MAKE_SHARED_SIPI)
    add_library(unistring SHARED IMPORTED GLOBAL)
    set_property(TARGET unistring PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libunistring${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(unistring STATIC IMPORTED GLOBAL)
    set_property(TARGET unistring PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libunistring${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(unistring project_unistring)
