cmake_minimum_required(VERSION 2.8.10)
project(freetype_builder C CXX)
include(ExternalProject)
#
# get png
#
SET(CMAKE_PREFIX_PATH ${COMMON_LOCAL} ${CMAKE_PREFIX_PATH})
ExternalProject_Add(
        project_freetype
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://sourceforge.net/projects/freetype/files/latest/download
        DOWNLOAD_NAME freetype-latest.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/freetype
        CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=false -DCMAKE_PREFIX_PATH=${COMMON_LOCAL} -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
)
ExternalProject_Get_Property(project_freetype install_dir)
if(MAKE_SHARED_SIPI)
    add_library(freetype SHARED IMPORTED GLOBAL)
    set_property(TARGET freetype PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfreetype${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(freetype STATIC IMPORTED GLOBAL)
    set_property(TARGET freetype PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfreetype${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(freetype project_freetype)
