cmake_minimum_required(VERSION 2.8.10)
project(libluarocks_builder C CXX)
include(ExternalProject)

#
# get luarocks
#
ExternalProject_Add(project_luarocks
        DEPENDS lua
        INSTALL_DIR ${COMMON_LOCAL}
        URL http://keplerproject.github.io/luarocks/releases/luarocks-2.3.0.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/luarocks-2.3.0
        CONFIGURE_COMMAND ${COMMON_SRCS}/luarocks-2.3.0/configure
            --prefix=${COMMON_LOCAL}
            --with-lua-bin=${COMMON_LOCAL}/bin
            --with-lua-include=${COMMON_LOCAL}/include
            --with-lua-lib=${COMMON_LOCAL}/lib
            --rocks-tree=${CMAKE_INSTALL_PREFIX}/luarocks
        BUILD_COMMAND make build
        INSTALL_COMMAND make install
        BUILD_IN_SOURCE 1
        )
#ExternalProject_Get_Property(project_luarocks install_dir)

#if(MAKE_SHARED_SIPI)
#    add_library(luarocks SHARED IMPORTED GLOBAL)
#    set_property(TARGET luarocks PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libexiv2${CMAKE_SHARED_LIBRARY_SUFFIX})
#else()
#    add_library(luarocks STATIC IMPORTED GLOBAL)
#    set_property(TARGET luarocks PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libexiv2${CMAKE_STATIC_LIBRARY_SUFFIX})
#endif()
#add_dependencies(luarocks project_luarocks)
