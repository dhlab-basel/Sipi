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
        URL http://download.osgeo.org/libtiff/tiff-4.0.6.tar.gz
        SOURCE_DIR ${COMMON_LIB}/libtiff-4.0.6
        UPDATE_COMMAND patch ${COMMON_LIB}/libtiff-4.0.6/libtiff/tif_dirinfo.c < ${COMMON_PATCHES}/tiff-4.0.4.patch
        CMAKE_ARGS ${CMAKE_CREATE_SHARED} -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} ${COMMON_LOCAL}/lib
        INSTALL_COMMAND make test
        COMMAND make install
        COMMAND ${CMAKE_COMMAND} -E copy ${COMMON_LIB}/libtiff-4.0.6/libtiff/tif_dir.h ${COMMON_LOCAL}/include/
        BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_tiff install_dir)
if(MAKE_SHARED_SIPI)
    add_library(tiff SHARED IMPORTED GLOBAL)
    set_property(TARGET tiff PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libtiff${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(tiff STATIC IMPORTED GLOBAL)
    MESSAGE(STATUS "install dir" ${install_dir})
    if(CMAKE_SYSTEM_NAME STREQUAL DARWIN)
       set_property(TARGET tiff PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libtiff${CMAKE_STATIC_LIBRARY_SUFFIX})
    elseif(CMAKE_SYSTEM_NAME STREQUAL LINUX)
		if(CMAKE_DISTRO_NAME STREQUAL Ubuntu)
       		set_property(TARGET tiff PROPERTY IMPORTED_LOCATION ${install_dir}/lib/x86_64-linux-gnu/libtiff${CMAKE_STATIC_LIBRARY_SUFFIX})
 #          set_property(TARGET tiff PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libtiff${CMAKE_STATIC_LIBRARY_SUFFIX})
		else()
       		set_property(TARGET tiff PROPERTY IMPORTED_LOCATION ${install_dir}/lib64/libtiff${CMAKE_STATIC_LIBRARY_SUFFIX})
    	endif()
    endif()
endif()
add_dependencies(tiff project_tiff)
