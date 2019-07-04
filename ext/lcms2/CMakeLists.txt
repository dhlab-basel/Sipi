cmake_minimum_required(VERSION 2.8.10)
project(lcms2_builder C CXX)
include(ExternalProject)

#
# get lcms2 Version 2.8
#
ExternalProject_Add(project_lcms2
    INSTALL_DIR ${COMMON_LOCAL}
    URL https://dl.bintray.com/dhlab-basel/thirdparty/lcms2-2.9.tar.gz
    URL_HASH SHA256=48c6fdf98396fa245ed86e622028caf49b96fa22f3e5734f853f806fbc8e7d20
    SOURCE_DIR ${COMMON_SRCS}/lcms2-2.9
    CONFIGURE_COMMAND ${COMMON_SRCS}/lcms2-2.9/configure --prefix=${COMMON_LOCAL} --with-jpeg=${COMMON_LOCAL} --with-tiff=${COMMON_LOCAL} --libdir=${CONFIGURE_LIBDIR}
    BUILD_COMMAND make
    BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_lcms2 install_dir)
if(MAKE_SHARED_SIPI)
    add_library(lcms2 SHARED IMPORTED GLOBAL)
    set_property(TARGET lcms2 PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/liblcms2${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(lcms2 STATIC IMPORTED GLOBAL)
    set_property(TARGET lcms2 PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/liblcms2${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(lcms2 project_lcms2)
