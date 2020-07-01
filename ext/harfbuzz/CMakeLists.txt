cmake_minimum_required(VERSION 3.14.5)
project(harfbuzz_builder C CXX)
include(ExternalProject)

#
# get harfbuzz
#

ExternalProject_Add(project_harfbuzz
        DEPENDS zlib
        DEPENDS png
        DEPENDS freetype
        DEPENDS bzip2
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://www.freedesktop.org/software/harfbuzz/release/harfbuzz-2.6.4.tar.xz
        SOURCE_DIR ${COMMON_SRCS}/harfbuzz-2.6.4
        CMAKE_ARGS  -DBUILD_SHARED_LIBS:BOOL=OFF
                    -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
                    -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
                    -DBUILD_FRAMEWORK:BOOL=OFF
                    -DHB_HAVE_CORETEXT:BOOL=OFF
                    -DHB_BUILD_TESTS:BOOL=OFF
                    -DHB_HAVE_FREETYPE:BOOL=ON
                    -DCMAKE_PREFIX_PATH=${COMMON_LOCAL}
                    -DCMAKE_LIBRARY_PATH=${CONFIGURE_LIBDIR}
        )

ExternalProject_Get_Property(project_harfbuzz install_dir)

# static
add_library(harfbuzz STATIC IMPORTED GLOBAL)
set_property(TARGET harfbuzz PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libharfbuzz${CMAKE_STATIC_LIBRARY_SUFFIX})

add_dependencies(harfbuzz project_harfbuzz)
