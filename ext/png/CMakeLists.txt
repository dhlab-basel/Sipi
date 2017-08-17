cmake_minimum_required(VERSION 2.8.10)
project(libpng_builder C CXX)
include(ExternalProject)

#
# get png
#
ExternalProject_Add(
        project_png
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://sourceforge.net/projects/libpng/files/libpng16/older-releases/1.6.29/libpng-1.6.29.tar.gz/download
        DOWNLOAD_NAME libpng-latest.tar.gz
        #URL https://sourceforge.net/projects/libpng/files/latest/download?source=files
        SOURCE_DIR ${COMMON_SRCS}/libpng-latest
        CMAKE_ARGS -DPNG_SHARED:BOOL=OFF -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
)
ExternalProject_Get_Property(project_png install_dir)
if(MAKE_SHARED_SIPI)
    add_library(png SHARED IMPORTED GLOBAL)
    set_property(TARGET png PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libpng${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(png STATIC IMPORTED GLOBAL)
    set_property(TARGET png PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libpng${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(png project_png)
