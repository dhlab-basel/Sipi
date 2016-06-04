cmake_minimum_required(VERSION 2.8.10)
project(libpng_builder C CXX)
include(ExternalProject)

#
# get png
#
ExternalProject_Add(project_png
    INSTALL_DIR ${COMMON_LOCAL}
    URL http://sourceforge.net/projects/libpng/files/libpng16/1.6.21/libpng-1.6.21.tar.gz/download
    SOURCE_DIR ${COMMON_LIB}/libpng-1.6.20
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
)
ExternalProject_Get_Property(project_png install_dir)
if(MAKE_SHARED_SIPI)
    add_library(png SHARED IMPORTED GLOBAL)
    set_property(TARGET png PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libpng${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(png STATIC IMPORTED GLOBAL)
    set_property(TARGET png PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libpng${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(png project_png)

