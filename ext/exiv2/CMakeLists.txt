cmake_minimum_required(VERSION 2.8.10)
project(exiv2_builder C CXX)
include(ExternalProject)


#
# get exiv2
#
#if(MAKE_SHARED_SIPI)
#    set(XMPDSK_LIBFILE "libxmpsdk${CMAKE_STATIC_LIBRARY_SUFFIX}")
#else()
#    set(XMPDSK_LIBFILE "libxmpsdk${CMAKE_STATIC_LIBRARY_SUFFIX}")
#endif()
ExternalProject_Add(project_exiv2
        DEPENDS expat
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://dl.bintray.com/dhlab-basel/thirdparty/exiv2-0.26-trunk.tar.gz
        URL_HASH SHA256=c75e3c4a0811bf700d92c82319373b7a825a2331c12b8b37d41eb58e4f18eafb
        SOURCE_DIR ${COMMON_SRCS}/exiv2-trunk
        CONFIGURE_COMMAND ${COMMON_SRCS}/exiv2-trunk/configure --prefix=${COMMON_LOCAL} --with-expat=${COMMON_LOCAL} --libdir=${CONFIGURE_LIBDIR}
        BUILD_COMMAND make
        BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_exiv2 install_dir)

if(MAKE_SHARED_SIPI)
    add_library(exiv2 SHARED IMPORTED GLOBAL)
    set_property(TARGET exiv2 PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libexiv2${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(exiv2 STATIC IMPORTED GLOBAL)
    set_property(TARGET exiv2 PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libexiv2${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(exiv2 project_exiv2)
