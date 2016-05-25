cmake_minimum_required(VERSION 2.8.10)
project(jbigkit_builder C CXX)
include(ExternalProject)

#
# get JBIG
#
ExternalProject_Add(
        project_jbigkit
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://www.cl.cam.ac.uk/~mgk25/jbigkit/download/jbigkit-2.1.tar.gz
        SOURCE_DIR ${COMMON_LIB}//jbigkit-2.1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy ${COMMON_LIB}//jbigkit-2.1/libjbig/libjbig${CMAKE_STATIC_LIBRARY_SUFFIX} ${COMMON_LOCAL}/lib
        BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_jbigkit install_dir)
add_library(jbigkit STATIC IMPORTED GLOBAL)
set_property(TARGET jbigkit PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libjbig${CMAKE_STATIC_LIBRARY_SUFFIX})
set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")
add_dependencies(jbigkit project_jbigkit)
