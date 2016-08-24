cmake_minimum_required(VERSION 2.8.10)
project(kakadu_builder C CXX)
include(ExternalProject)

#
# compiling kakadu
#
#FindJava()
ExternalProject_Add(
        project_kakadu
        INSTALL_DIR ${COMMON_LOCAL}
        DOWNLOAD_COMMAND unzip -o -d ${COMMON_LIB} ${COMMON_VENDOR}/v7_8-01382N.zip
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make --directory=${COMMON_LIB}/v7_8-01382N/make/ -f ${KDU_MAKE}
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory ${COMMON_LIB}/v7_8-01382N/lib/${KDU_ARCH} ${COMMON_LOCAL}/lib
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${COMMON_LIB}/v7_8-01382N/bin/${KDU_ARCH} ${COMMON_LOCAL}/bin
        COMMAND  ${CMAKE_COMMAND} -E copy_directory ${COMMON_LIB}/v7_8-01382N/managed/all_includes ${COMMON_LOCAL}/include
)
ExternalProject_Get_Property(project_kakadu install_dir)
if(MAKE_SHARED_SIPI)
    add_library(kdu SHARED IMPORTED GLOBAL)
    set_property(TARGET kdu PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libkdu_v78R.so)
else()
    add_library(kdu STATIC IMPORTED GLOBAL)
    set_property(TARGET kdu PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libkdu${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(kdu project_kakadu)
if(MAKE_SHARED_SIPI)
    add_library(kdu_aux SHARED IMPORTED GLOBAL)
    set_property(TARGET kdu_aux PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libkdu_a78R.so)
else()
    add_library(kdu_aux STATIC IMPORTED GLOBAL)
    set_property(TARGET kdu_aux PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libkdu_aux${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(kdu_aux project_kakadu)
