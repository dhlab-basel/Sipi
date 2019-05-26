cmake_minimum_required(VERSION 2.8.10)
project(gettext_builder C CXX)
include(ExternalProject)

#
# get gettext
#
ExternalProject_Add(
        project_gettext
        INSTALL_DIR ${COMMON_LOCAL}
        URL http://ftp.gnu.org/pub/gnu/gettext/gettext-0.20.1.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/gettext--0.20.1
        CONFIGURE_COMMAND ${COMMON_SRCS}/gettext--0.20.1/configure --prefix=${COMMON_LOCAL} --enable-static --libdir=${CONFIGURE_LIBDIR}
        BUILD_COMMAND make
        #INSTALL_COMMAND ${CMAKE_COMMAND} -E copy ${COMMON_SRCS}/gettext-latest/gettext-runtime/intl/.libs/libintl${CMAKE_STATIC_LIBRARY_SUFFIX} ${CONFIGURE_LIBDIR}
        BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_gettext install_dir)
add_library(gettext STATIC IMPORTED GLOBAL)
#set_property(TARGET gettext PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libintl${CMAKE_STATIC_LIBRARY_SUFFIX})
set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")

add_dependencies(gettext project_gettext)
