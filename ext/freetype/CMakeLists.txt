cmake_minimum_required(VERSION 2.8.10)
project(freetytpe_builder C CXX)
include(ExternalProject)

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
        /usr/include
)

ExternalProject_Add(project_freetype
        INSTALL_DIR ${COMMON_LOCAL}
        #URL_HASH SHA256=c75e3c4a0811bf700d92c82319373b7a825a2331c12b8b37d41eb58e4f18eafb
        URL https://download.savannah.gnu.org/releases/freetype/freetype-2.9.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/freetype-2.9

        CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}

#        BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_freetype install_dir)

if(MAKE_SHARED_SIPI)
    add_library(freetype SHARED IMPORTED GLOBAL)
    set_property(TARGET freetype PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfreetype${CMAKE_SHARED_LIBRARY_SUFFIX})
    set( ENV{FREETYPE_LIBS} -L${CONFIGURE_LIBDIR}/libfreetype${CMAKE_SHARED_LIBRARY_SUFFIX} )
else()
    add_library(freetype STATIC IMPORTED GLOBAL)
    set_property(TARGET freetype PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfreetype${CMAKE_STATIC_LIBRARY_SUFFIX})
    set( ENV{FREETYPE_LIBS} -L${CONFIGURE_LIBDIR}/libfreetype${CMAKE_STATIC_LIBRARY_SUFFIX} )
endif()

set( ENV{FREETYPE_CFLAGS} -I${COMMON_LOCAL}/incluse/freetype2 )

add_dependencies(freetype project_freetype)
