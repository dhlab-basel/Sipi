cmake_minimum_required(VERSION 2.8.10)
project(jpeg_builder C CXX)
include(ExternalProject)

#
# get libjpeg 1.4.1
#
ExternalProject_Add(project_jpeg
    INSTALL_DIR ${COMMON_LOCAL}
    URL http://www.ijg.org/files/jpegsrc.v9b.tar.gz
    SOURCE_DIR ${COMMON_SRCS}/libjpeg-v9b
    CONFIGURE_COMMAND ${COMMON_SRCS}/libjpeg-v9b/configure --prefix=${COMMON_LOCAL} --enable-shared --enable-static --libdir=${CONFIGURE_LIBDIR}
    BUILD_COMMAND make
    BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_jpeg install_dir)
if(MAKE_SHARED_SIPI)
    add_library(jpeg SHARED IMPORTED GLOBAL)
    set_property(TARGET jpeg PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libjpeg${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=ON")
else()
    add_library(jpeg STATIC IMPORTED GLOBAL)
    set_property(TARGET jpeg PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libjpeg${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")
endif()
add_dependencies(jpeg project_jpeg)
