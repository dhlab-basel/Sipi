cmake_minimum_required(VERSION 3.14.5)
project(poppler_builder C CXX)
include(ExternalProject)

ExternalProject_Add(project_poppler
        DEPENDS png
        DEPENDS jpeg
        DEPENDS curl
        DEPENDS zlib
        DEPENDS fontconfig
        DEPENDS freetype
        DEPENDS lcms2
        DEPENDS expat
        DEPENDS bzip2
        INSTALL_DIR ${COMMON_LOCAL}
        link_directories(${CONFIGURE_LIBDIR})
        COMMAND cd ${COMMON_LOCAL}; git clone git://git.freedesktop.org/git/poppler/test
        #URL_HASH SHA256=c75e3c4a0811bf700d92c82319373b7a825a2331c12b8b37d41eb58e4f18eafb
        URL https://poppler.freedesktop.org/poppler-0.80.0.tar.xz
        SOURCE_DIR ${COMMON_SRCS}/poppler
        PATCH_COMMAND patch -d ${COMMON_SRCS}/poppler -p0 < ${COMMON_PATCHES}/CMakeLists.poppler.patch
        CMAKE_ARGS  -DENABLE_CMS=lcms2
                    -DENABLE_DCTDECODER=libjpeg
                    -DENABLE_LIBOPENJPEG=none
                    -DENABLE_UTILS:BOOL=OFF
                    -DBUILD_SHARED_LIBS:BOOL=OFF
                    -DBUILD_GTK_TESTS:BOOL=OFF
                    -DBUILD_QT5_TESTS:BOOL=OFF
                    -DBUILD_CPP_TESTS:BOOL=OFF
                    -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
                    -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
)

ExternalProject_Get_Property(project_poppler install_dir)

# static
add_library(poppler STATIC IMPORTED GLOBAL)
set_property(TARGET poppler PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libpoppler${CMAKE_STATIC_LIBRARY_SUFFIX})

add_library(poppler-cpp STATIC IMPORTED GLOBAL)
set_property(TARGET poppler-cpp PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libpoppler-cpp${CMAKE_STATIC_LIBRARY_SUFFIX})

add_dependencies(poppler project_poppler)

# shared
#add_library(poppler_shared SHARED IMPORTED GLOBAL)
#set_property(TARGET poppler PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libpoppler${CMAKE_SHARED_LIBRARY_SUFFIX})

#add_library(poppler-cpp_shared SHARED IMPORTED GLOBAL)
#set_property(TARGET poppler-cpp PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libpoppler-cpp${CMAKE_SHARED_LIBRARY_SUFFIX})

#add_dependencies(poppler_shared project_poppler)
