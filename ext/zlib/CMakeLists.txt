cmake_minimum_required(VERSION 2.8.10)
project(zlib_builder C CXX)
include(ExternalProject)

#
# zlib
#
ExternalProject_Add(project_zlib
        INSTALL_DIR ${COMMON_LOCAL}
        URL http://zlib.net/zlib-1.2.8.tar.gz
        SOURCE_DIR ${COMMON_LIB}/zlib-1.2.8
        CONFIGURE_COMMAND ${COMMON_LIB}/zlib-1.2.8/configure --prefix=${COMMON_LOCAL} --static --shared
        BUILD_COMMAND make
        BUILD_IN_SOURCE 1
        )
ExternalProject_Get_Property(project_zlib install_dir)
if(MAKE_SHARED_SIPI)
    add_library(z SHARED IMPORTED GLOBAL)
    set_property(TARGET z PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libz${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(z STATIC IMPORTED GLOBAL)
    set_property(TARGET z PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libz${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(z project_zlib)
