cmake_minimum_required(VERSION 2.8.10)
project(freetype_builder C CXX)
include(ExternalProject)

#
# get png
#
ExternalProject_Add(
        project_freetype
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://sourceforge.net/projects/freetype/files/latest/download
        DOWNLOAD_NAME freetype-latest.tar.gz
        #URL https://sourceforge.net/projects/libpng/files/latest/download?source=files
        SOURCE_DIR ${COMMON_SRCS}/freetype
        CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=false -DCMAKE_PREFIX_PATH=${COMMON_LOCAL} -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
)
ExternalProject_Get_Property(project_freetype install_dir)
if(MAKE_SHARED_SIPI)
    add_library(libfreetype SHARED IMPORTED GLOBAL)
    set_property(TARGET libfreetype PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfreetype${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(libfreetype STATIC IMPORTED GLOBAL)
    set_property(TARGET libfreetype PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfreetype${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(libfreetype project_libfreetype)
