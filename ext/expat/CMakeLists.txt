cmake_minimum_required(VERSION 2.8.10)
project(expat_builder C CXX)
include(ExternalProject)

ExternalProject_Add(project_expat
    INSTALL_DIR ${COMMON_LOCAL}
    URL http://sourceforge.net/projects/expat/files/expat/2.1.0/expat-2.1.0.tar.gz/download
    SOURCE_DIR ${COMMON_LIB}/expat
    CONFIGURE_COMMAND ${COMMON_LIB}/expat/configure --prefix=${COMMON_LOCAL}
    BUILD_COMMAND make
    BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_expat install_dir)
if(MAKE_SHARED_SIPI)
    add_library(expat SHARED IMPORTED GLOBAL)
    set_property(TARGET expat PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libexpat${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(expat STATIC IMPORTED GLOBAL)
    set_property(TARGET expat PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libexpat${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(expat project_expat)


