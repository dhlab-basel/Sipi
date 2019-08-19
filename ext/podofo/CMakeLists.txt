cmake_minimum_required(VERSION 3.14.5)
project(podofo_builder C CXX)
include(ExternalProject)

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
        /usr/include
)

ExternalProject_Add(project_podofo
        DEPENDS freetype
        DEPENDS fontconfig
        INSTALL_DIR ${COMMON_LOCAL}
        #URL_HASH SHA256=c75e3c4a0811bf700d92c82319373b7a825a2331c12b8b37d41eb58e4f18eafb
        URL https://sourceforge.net/projects/podofo/files/latest/download
        SOURCE_DIR ${COMMON_SRCS}/podofo
        CMAKE_ARGS -DPODOFO_BUILD_LIB_ONLY:BOOL=TRUE -DPODOFO_BUILD_STATIC:BOOL=TRUE -DPODOFO_BUILD_SHARED:BOOL=TRUE -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
        BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_podofo install_dir)

if(MAKE_SHARED_SIPI)
    add_library(podofo SHARED IMPORTED GLOBAL)
    set_property(TARGET podofo PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libpodofo${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(podofo STATIC IMPORTED GLOBAL)
    set_property(TARGET podofo PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libpodofo${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(podofo project_podofo)
