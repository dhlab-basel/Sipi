cmake_minimum_required(VERSION 2.8.10)
project(libxz_builder C CXX)
include(ExternalProject)

#
# libxyz
#
ExternalProject_Add(project_xz
    INSTALL_DIR ${COMMON_LOCAL}
    URL http://tukaani.org/xz/xz-5.2.1.tar.gz
    SOURCE_DIR ${COMMON_SRCS}/xz-5.2.1
    CONFIGURE_COMMAND ${COMMON_SRCS}/xz-5.2.1/configure --prefix=${COMMON_LOCAL} --libdir=${CONFIGURE_LIBDIR}
    BUILD_COMMAND make
    BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_xz install_dir)
if(MAKE_SHARED_SIPI)
    add_library(xz SHARED IMPORTED GLOBAL)
    set_property(TARGET xz PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/liblzma${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(xz STATIC IMPORTED GLOBAL)
    set_property(TARGET xz PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/liblzma${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(xz project_xz)
