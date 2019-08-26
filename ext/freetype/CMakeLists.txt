cmake_minimum_required(VERSION 3.14.5)
project(freetytpe_builder C CXX)
include(ExternalProject)

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
        /usr/include
)

# cmake output
# -- Could NOT find harfbuzz (missing: HARFBUZZ_INCLUDE_DIRS HARFBUZZ_LIBRARIES)

#
# static
#
ExternalProject_Add(project_freetype
        DEPENDS png
        DEPENDS bzip2
        INSTALL_DIR ${COMMON_LOCAL}
        #URL_HASH SHA256=c75e3c4a0811bf700d92c82319373b7a825a2331c12b8b37d41eb58e4f18eafb
        URL https://download.savannah.gnu.org/releases/freetype/freetype-2.10.1.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/freetype-2.10.1
        CMAKE_ARGS  -DBUILD_SHARED_LIBS:BOOL=OFF
                    -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
                    -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
                    -DPNG_LIBRARY=${CONFIGURE_LIBDIR}/libpng.o
                    -DPNG_PNG_INCLUDE_DIR=${CONFIGURE_INCDIR}
                    -DBZIP2_LIBRARIES=${CONFIGURE_LIBDIR}/libbz2.o
                    -DBZIP2_INCLUDE_DIR=${CONFIGURE_INCDIR}

        #        BUILD_IN_SOURCE 1
        )
ExternalProject_Get_Property(project_freetype install_dir)

add_library(freetype STATIC IMPORTED GLOBAL)
set_property(TARGET freetype PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfreetype${CMAKE_STATIC_LIBRARY_SUFFIX})

add_dependencies(freetype project_freetype)

#
# shared
#
#ExternalProject_Add(project_freetype_shared
#        DEPENDS png_shared
#        DEPENDS bzip2_shared
#        INSTALL_DIR ${COMMON_LOCAL}
#        #URL_HASH SHA256=c75e3c4a0811bf700d92c82319373b7a825a2331c12b8b37d41eb58e4f18eafb
#        URL https://download.savannah.gnu.org/releases/freetype/freetype-2.10.1.tar.gz
#        SOURCE_DIR ${COMMON_SRCS}/freetype-2.10.1
#        CMAKE_ARGS  -DBUILD_SHARED_LIBS:BOOL=ON
#                    -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
#                    -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
#                    -DPNG_LIBRARY=${CONFIGURE_LIBDIR}/libpng${CMAKE_SHARED_LIBRARY_SUFFIX}
#                    -DPNG_PNG_INCLUDE_DIR=${CONFIGURE_INCDIR}
#                    -DBZIP2_LIBRARIES=${CONFIGURE_LIBDIR}/libbz2${CMAKE_SHARED_LIBRARY_SUFFIX}
#                    -DBZIP2_INCLUDE_DIR=${CONFIGURE_INCDIR}
#
##        BUILD_IN_SOURCE 1
#)
#ExternalProject_Get_Property(project_freetype_shared install_dir)
#
## shared
#add_library(freetype_shared SHARED IMPORTED GLOBAL)
#set_property(TARGET freetype_shared PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfreetype${CMAKE_SHARED_LIBRARY_SUFFIX})
#
#add_dependencies(freetype_shared project_freetype_shared)
