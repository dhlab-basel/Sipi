cmake_minimum_required(VERSION 2.8.10)
project(libpng_builder C CXX)
include(ExternalProject)

#
# get png
#
ExternalProject_Add(
        project_png
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://dl.bintray.com/dhlab-basel/thirdparty/libpng-1.6.32.tar.gz
        URL_HASH SHA256=1a8ae5c8eafad895cc3fce78fbcb6fdef663b8eb8375f04616e6496360093abb
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
