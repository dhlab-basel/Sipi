cmake_minimum_required(VERSION 2.8.10)
project(pdfwriter_builder C CXX)
include(ExternalProject)

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
        /usr/include
)

ExternalProject_Add(project_pdfwriter
        INSTALL_DIR ${COMMON_LOCAL}
        #URL https://dl.bintray.com/dhlab-basel/thirdparty/exiv2-0.26-trunk.tar.gz
        #URL_HASH SHA256=c75e3c4a0811bf700d92c82319373b7a825a2331c12b8b37d41eb58e4f18eafb
        GIT_REPOSITORY https://github.com/galkahana/PDF-Writer.git
        SOURCE_DIR ${COMMON_SRCS}/PDF-Writer

        #CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF -DEXIV2_ENABLE_VIDEO=ON -DCMAKE_MODULE_PATH=${COMMON_LOCAL} -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
        CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}

        BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_pdfwriter install_dir)

if(MAKE_SHARED_SIPI)
    add_library(pdfwriter SHARED IMPORTED GLOBAL)
    set_property(TARGET pdfwriter PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libPDFWriter${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(pdfwriter STATIC IMPORTED GLOBAL)
    set_property(TARGET pdfwriter PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libPDFWriter${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
add_dependencies(pdfwriter project_pdfwriter)
