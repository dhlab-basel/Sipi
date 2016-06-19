cmake_minimum_required(VERSION 2.8.10)
project(libjansson_builder C CXX)
include(ExternalProject)

#
# get jansson
#
ExternalProject_Add(
        project_jansson
        INSTALL_DIR ${COMMON_LOCAL}
        GIT_REPOSITORY https://github.com/akheron/jansson.git
        SOURCE_DIR ${COMMON_LIB}/jansson
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
)
ExternalProject_Get_Property(project_jansson install_dir)
add_library(jansson STATIC IMPORTED GLOBAL)
set_property(TARGET jansson PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libjansson${CMAKE_STATIC_LIBRARY_SUFFIX})
add_dependencies(jansson project_jansson)
