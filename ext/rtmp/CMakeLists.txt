cmake_minimum_required(VERSION 2.8.10)
project(rtmp_builder C CXX)
include(ExternalProject)

#
# get librtmp
#
ExternalProject_Add(project_rtmp
        INSTALL_DIR ${COMMON_LOCAL}
        URL http://rtmpdump.mplayerhq.hu/download/rtmpdump-2.3.tgz
        SOURCE_DIR ${COMMON_SRCS}/rtmpdump-2.3
        BUILD_COMMAND make
        BUILD_IN_SOURCE 1
        )
ExternalProject_Get_Property(project_rtmp install_dir)
if(MAKE_SHARED_SIPI)
    add_library(rtmp SHARED IMPORTED GLOBAL)
    set_property(TARGET rtmp PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/librtmp${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=ON")
else()
    add_library(rtmp STATIC IMPORTED GLOBAL)
    set_property(TARGET rtmp PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/librtmp${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")
endif()
add_dependencies(rtmp project_rtmp)
