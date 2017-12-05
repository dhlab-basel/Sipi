cmake_minimum_required(VERSION 2.8.10)
project(kakadu_builder C CXX)
include(ExternalProject)

message(STATUS ${CONFIGURE_LIBDIR})
#
# compiling kakadu
#
#FindJava()
ExternalProject_Add(
        project_kakadu
        INSTALL_DIR ${COMMON_LOCAL}
        DOWNLOAD_COMMAND unzip -o -d ${COMMON_SRCS} ${COMMON_VENDOR}/v7_A_2-01382N.zip
        PATCH_COMMAND patch -d ${COMMON_SRCS}/v7_A_2-01382N -p0 < ${COMMON_PATCHES}/${KDU_MAKE}.patch
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make --directory=${COMMON_SRCS}/v7_A_2-01382N/make/ -f ${KDU_MAKE}
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory ${COMMON_SRCS}/v7_A_2-01382N/lib/${KDU_ARCH} ${CONFIGURE_LIBDIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${COMMON_SRCS}/v7_A_2-01382N/bin/${KDU_ARCH} ${COMMON_LOCAL}/bin
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${COMMON_SRCS}/v7_A_2-01382N/managed/all_includes ${COMMON_LOCAL}/include
)

ExternalProject_Get_Property(project_kakadu install_dir)
if(MAKE_SHARED_SIPI)
    add_library(kdu SHARED IMPORTED GLOBAL)
    set_property(TARGET kdu PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libkdu_v7AR.so)
else()
    add_library(kdu STATIC IMPORTED GLOBAL)
    set_property(TARGET kdu PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libkdu${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(kdu project_kakadu)
if(MAKE_SHARED_SIPI)
    add_library(kdu_aux SHARED IMPORTED GLOBAL)
    set_property(TARGET kdu_aux PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libkdu_a7AR.so)
else()
    add_library(kdu_aux STATIC IMPORTED GLOBAL)
    set_property(TARGET kdu_aux PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libkdu_aux${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(kdu_aux project_kakadu)
