cmake_minimum_required(VERSION 3.14.5)
project(poppler_builder C CXX)
include(ExternalProject)

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
        /usr/include
)

ExternalProject_Add(project_poppler
        DEPENDS png
        DEPENDS jpeg
        DEPENDS curl
        DEPENDS zlib
        DEPENDS fontconfig
        DEPENDS freetype
        DEPENDS lcms2
        DEPENDS expat
        INSTALL_DIR ${COMMON_LOCAL}
        COMMAND cd ${COMMON_LOCAL}; git clone git://git.freedesktop.org/git/poppler/test
        #URL_HASH SHA256=c75e3c4a0811bf700d92c82319373b7a825a2331c12b8b37d41eb58e4f18eafb
        URL https://poppler.freedesktop.org/poppler-0.74.0.tar.xz
        SOURCE_DIR ${COMMON_SRCS}/poppler
        CMAKE_ARGS
            -DENABLE_CMS=lcms2
            -DENABLE_DCTDECODER=libjpeg
            -DENABLE_LIBOPENJPEG=none
            -DBUILD_SHARED_LIBS:BOOL=FALSE
            -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
            # -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
            -DFREETYPE_INCLUDE_DIR_freetype2=${CONFIGURE_INCDIR}/freetype2
            -DFREETYPE_INCLUDE_DIR_ft2build=${CONFIGURE_INCDIR}
            -DFREETYPE_LIBRARY=${CONFIGURE_LIBDIR}/libfreetype.so
        BUILD_IN_SOURCE 1
)

ExternalProject_Get_Property(project_poppler install_dir)

if(MAKE_SHARED_SIPI)
    add_library(poppler SHARED IMPORTED GLOBAL)
    set_property(TARGET poppler PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libpoppler${CMAKE_SHARED_LIBRARY_SUFFIX})
    add_library(poppler-cpp SHARED IMPORTED GLOBAL)
    set_property(TARGET poppler-cpp PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libpoppler-cpp${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(poppler STATIC IMPORTED GLOBAL)
    set_property(TARGET poppler PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libpoppler${CMAKE_STATIC_LIBRARY_SUFFIX})
    add_library(poppler-cpp STATIC IMPORTED GLOBAL)
    set_property(TARGET poppler-cpp PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libpoppler-cpp${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(poppler project_poppler)
