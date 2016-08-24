cmake_minimum_required(VERSION 2.8.10)
project(liblibmagic_builder C CXX)
include(ExternalProject)

#
# get libmagic
#

Externalproject_Add(
        project_magic
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://github.com/threatstack/libmagic/archive/master.zip
        SOURCE_DIR ${COMMON_SRCS}/libmagic-master
        CONFIGURE_COMMAND ${COMMON_SRCS}/libmagic-master/configure --prefix=${COMMON_LOCAL}  --enable-shared --enable-static
        BUILD_COMMAND make
        INSTALL_COMMAND make install INSTALL_TOP=${COMMON_LOCAL}
        BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_magic install_dir)
if(MAKE_SHARED_SIPI)
    add_library(magic SHARED IMPORTED GLOBAL)
    set_property(TARGET magic PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libmagic${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(magic STATIC IMPORTED GLOBAL)
    set_property(TARGET magic PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libmagic${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(magic project_magic)

