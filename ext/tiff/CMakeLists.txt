cmake_minimum_required(VERSION 2.8.10)
project(tiff_builder C CXX)
include(ExternalProject)

#
# get libtiff-4.0.6
#
if(MAKE_SHARED_SIPI)
    SET(TIFF_LIBRARY "libtiff${CMAKE_SHARED_LIBRARY_SUFFIX}")
    SET(TIFFXX_LIBRARY "libtiffxx${CMAKE_SHARED_LIBRARY_SUFFIX}")
else()
    SET(TIFF_LIBRARY "libtiff${CMAKE_STATIC_LIBRARY_SUFFIX}")
    SET(TIFFXX_LIBRARY "libtiffxx${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")
endif()
ExternalProject_Add(project_tiff
        DEPENDS project_jpeg project_xz
        INSTALL_DIR ${COMMON_LOCAL}
        TEST_BEFORE_INSTALL 1
        URL http://download.osgeo.org/libtiff/tiff-4.0.9.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/libtiff-4.0.9
        PATCH_COMMAND patch ${COMMON_SRCS}/libtiff-4.0.9/libtiff/tif_dirinfo.c < ${COMMON_PATCHES}/tiff-4.0.4.patch
        CMAKE_ARGS ${CMAKE_CREATE_SHARED} -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
        INSTALL_COMMAND make test
        COMMAND make install
        COMMAND ${CMAKE_COMMAND} -E copy ${COMMON_SRCS}/libtiff-4.0.9/libtiff/tif_dir.h ${COMMON_LOCAL}/include/
        BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_tiff install_dir)
if(MAKE_SHARED_SIPI)
    add_library(tiff SHARED IMPORTED GLOBAL)
    set_property(TARGET tiff PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libtiff${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(tiff STATIC IMPORTED GLOBAL)
    MESSAGE(STATUS "install dir" ${install_dir})
    set_property(TARGET tiff PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libtiff${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(tiff project_tiff)
