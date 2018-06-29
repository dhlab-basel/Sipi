cmake_minimum_required(VERSION 2.8.10)
project(libfmt_builder C CXX)
include(ExternalProject)

#
# get fmt
#
ExternalProject_Add(
        project_fmt
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://github.com/fmtlib/fmt/releases/download/5.0.0/fmt-5.0.0.zip
        SOURCE_DIR ${COMMON_SRCS}/fmt-5.0.0
        CMAKE_ARGS -DFMT_DOC=OFF -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
)
ExternalProject_Get_Property(project_fmt install_dir)
if(MAKE_SHARED_SIPI)
    add_library(fmt SHARED IMPORTED GLOBAL)
    set_property(TARGET fmt PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfmt${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(fmt STATIC IMPORTED GLOBAL)
    set_property(TARGET fmt PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfmt${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(fmt project_fmt)
