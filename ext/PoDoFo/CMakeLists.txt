cmake_minimum_required(VERSION 2.8.10)
project(podofo_builder C CXX)
include(ExternalProject)

#
# get png
#
ExternalProject_Add(
        project_podofo
        DEPENDS freetype
        DEPENDS fontconfig
        DEPENDS bzip2
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://sourceforge.net/projects/podofo/files/latest/download
        DOWNLOAD_NAME podofo-latest.tar.gz
        #URL https://sourceforge.net/projects/libpng/files/latest/download?source=files
        SOURCE_DIR ${COMMON_SRCS}/podofo
        CMAKE_ARGS -DCMAKE_PREFIX_PATH=${COMMON_LOCAL} -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
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
