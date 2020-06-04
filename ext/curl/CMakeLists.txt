cmake_minimum_required(VERSION 3.14.5)
project(curl_builder C CXX)
include(ExternalProject)

if (OPENSSL_ROOT_DIR)
    set(DARWIN "Darwin")
    set(LINUX "Linux")
    SET(OSSL  "--with-ssl=${OPENSSL_ROOT_DIR}")
else()
    SET(OSSL "")
endif()

#
# get libcurl
#
ExternalProject_Add(project_curl
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://curl.haxx.se/download/curl-7.68.0.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/curl-7.68.0
        CONFIGURE_COMMAND ${COMMON_SRCS}/curl-7.68.0/configure
            --prefix=${COMMON_LOCAL}
            --enable-shared
            --enable-static
            --libdir=${CONFIGURE_LIBDIR} ${OSSL}
            --disable-ldap
            --without-libidn2
        BUILD_COMMAND make
        BUILD_IN_SOURCE 1
        )
ExternalProject_Get_Property(project_curl install_dir)
if(MAKE_SHARED_SIPI)
    add_library(curl SHARED IMPORTED GLOBAL)
    set_property(TARGET curl PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libcurl${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=ON")
else()
    add_library(curl STATIC IMPORTED GLOBAL)
    set_property(TARGET curl PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libcurl${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")
endif()
add_dependencies(curl project_curl)
