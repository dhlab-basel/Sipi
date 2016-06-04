cmake_minimum_required(VERSION 2.8.10)
project(liblua_builder C CXX)
include(ExternalProject)


#
# get lua
#
ExternalProject_Add(project_lua
    INSTALL_DIR ${COMMON_LOCAL}
    URL http://www.lua.org/ftp/lua-5.3.1.tar.gz
    SOURCE_DIR ${COMMON_LIB}/lua-5.3.1
    CONFIGURE_COMMAND ls
    BUILD_COMMAND make ${LUA_ARCH}
    INSTALL_COMMAND make install INSTALL_TOP=${COMMON_LOCAL}
    BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_lua install_dir)
add_library(lua STATIC IMPORTED GLOBAL)
set_property(TARGET lua PROPERTY IMPORTED_LOCATION ${install_dir}/lib/liblua${CMAKE_STATIC_LIBRARY_SUFFIX})
add_dependencies(lua project_lua)
