/*
 * Copyright © 2016 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
 * Ivan Subotic, Tobias Schweizer, André Kilchenmann, and André Fatton.
 * This file is part of Sipi.
 * Sipi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Sipi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Additional permission under GNU AGPL version 3 section 7:
 * If you modify this Program, or any covered work, by linking or combining
 * it with Kakadu (or a modified version of that library) or Adobe ICC Color
 * Profiles (or a modified version of that library) or both, containing parts
 * covered by the terms of the Kakadu Software Licence or Adobe Software Licence,
 * or both, the licensors of this Program grant you additional permission
 * to convey the resulting work.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public
 * License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <algorithm>
#include <functional>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>      // Needed for memset

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "LuaServer.h"
#include "Connection.h"
#include "Server.h"

#include "sole.hpp"
#include "cJSON.h"

using namespace std;

static const char __file__[] = __FILE__;

static const char servertablename[] = "server";

namespace shttps {


    char luaconnection[] = "__shttpsconnection";

    /*!
     * Error handler for Lua errors!
     */
    static int dont_panic(lua_State *L) {
        cerr << "DON'T PANIC !!!!" << endl; // TODO: remove this debugging oputput if no longer necessary
        const char *luapanic = lua_tostring(L, -1);
        throw Error(__file__, __LINE__, string("Lua panic: ") + luapanic);
    }
    //=========================================================================

    /*!
     * Instantiates a Lua server
     */
    LuaServer::LuaServer() {
        if ((L = luaL_newstate()) == NULL) {
            throw new Error(__file__, __LINE__, "Couldn't start lua interpreter!");
        }
        luaL_openlibs(L);

        lua_atpanic(L, dont_panic);
    }
    //=========================================================================

    /*!
     * Instantiates a Lua server and directly executes the script file given
     *
     * \param[in] luafile A file containing a Lua script or a Lua code chunk
     */
    LuaServer::LuaServer(const string &luafile) {
        if ((L = luaL_newstate()) == NULL) {
            throw new Error(__file__, __LINE__, "Couldn't start lua interpreter!");
        }
        luaL_openlibs(L);

        lua_atpanic(L, dont_panic);

        if (!luafile.empty()) {
            if (luaL_loadfile(L, luafile.c_str()) != 0) {
                const char *luaerror = lua_tostring(L, -1);
                throw Error(__file__, __LINE__, string("Lua error: ") + luaerror);
            }
            if (lua_pcall(L, 0, LUA_MULTRET, 0) != 0) {
                const char *luaerror = lua_tostring(L, -1);
                throw Error(__file__, __LINE__, string("Lua error: ") + luaerror);
            }
        }
    }
    //=========================================================================

    /*!
     * Destroys the Lua server and free's all resources (garbage collectors are called here)
     */
    LuaServer::~LuaServer() {
        lua_close(L);
    }
    //=========================================================================

    /*!
     * Activates the the connection buffer. Optionally the bugger size and increment size can be given
     * LUA: server.setBuffer([bufsize][,incsize])
     */
    static int lua_setbuffer(lua_State *L) {
        int bufsize = 0;
        int incsize = 0;

        int top = lua_gettop(L);
        if (top > 0) {
            if (lua_isinteger(L, 1)) {
                bufsize = lua_tointeger(L, 1);
            }
            else {
                lua_pushstring(L,
                               "Lua-Error 'server.setbuffer([bufize][, incsize])': requires bufsize size as integer!");
                lua_error(L);
            }
        }
        if (top > 1) {
            if (lua_isinteger(L, 2)) {
                incsize = lua_tointeger(L, 2);
            }
            else {
                lua_pushstring(L,
                               "Lua-Error 'server.setbuffer([bufize][, incsize])': requires incsize size as integer!");
                lua_error(L);
            }
        }
        lua_pop(L, 2);

        lua_getglobal(L, luaconnection); // push onto stack
        Connection *conn = (Connection *) lua_touserdata(L, -1); // does not change the stack
        lua_remove(L, -1); // remove from stack

        if ((bufsize > 0) && (incsize > 0)) {
            conn->setBuffer(bufsize, incsize);
        }
        else if (bufsize > 0) {
            conn->setBuffer(bufsize);
        }
        else {
            conn->setBuffer();
        }

        return 0;
    }
    //=========================================================================

    /*!
     * Checks the filetype of a given filepath
     * LUA: server.fs.ftype("path")
     * RETURNS: "FILE", "DIRECTORY", "CHARDEV", "BLOCKDEV", "LINK", "SOCKET" or "UNKNOWN"
     */
    static int lua_fs_ftype(lua_State *L) {
        struct stat s;

        int top = lua_gettop(L);
        if (top < 1) {
            lua_pushstring(L, "Lua-error 'server.fs.ftype(filename): parameter missing!");
            lua_error(L);
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "Lua-error 'server.fs.ftype(filename): filename is not a string!");
            lua_error(L);
            return 0;
        }
        const char *filename = lua_tostring(L, 1);
        lua_pop(L, 1); // clear stack

        if (stat(filename, &s) != 0) {
            lua_pushstring(L, strerror(errno));
            lua_error(L);
            return 0;
        }

        if (S_ISREG(s.st_mode)) {
            lua_pushstring(L, "FILE");
        }
        else if (S_ISDIR(s.st_mode)) {
            lua_pushstring(L, "DIRECTORY");
        }
        else if (S_ISCHR(s.st_mode)) {
            lua_pushstring(L, "CHARDEV");
        }
        else if (S_ISBLK(s.st_mode)) {
            lua_pushstring(L, "BLOCKDEV");
        }
        else if (S_ISLNK(s.st_mode)) {
            lua_pushstring(L, "LINK");
        }
        else if (S_ISFIFO(s.st_mode)) {
            lua_pushstring(L, "FIFO");
        }
        else if (S_ISSOCK(s.st_mode)) {
            lua_pushstring(L, "SOCKET");
        }
        else {
            lua_pushstring(L, "UNKNOWN");
        }
        return 1;
    }
    //=========================================================================

    /*!
     * check if a file is readable
     * LUA: server.fs.is_readable(filepath)
     */
    static int lua_fs_is_readable(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 1) {
            lua_pushstring(L, "Lua-error 'server.fs.is_readable(filename): parameter missing!");
            lua_error(L);
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "Lua-error 'server.fs.is_readable(filename): filename is not a string!");
            lua_error(L);
            return 0;
        }
        const char *filename = lua_tostring(L, 1);
        lua_pop(L, 1); // clear stack

        if (access(filename, R_OK) == 0) {
            lua_pushboolean(L, true);
        }
        else {
            lua_pushboolean(L, false);
        }
        return 1;
    }
    //=========================================================================

    /*!
     * check if a file is writeable
     * LUA: server.fs.is_writeable(filepath)
     */
    static int lua_fs_is_writeable(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 1) {
            lua_pushstring(L, "Lua-error 'server.fs.is_writeable(filename): parameter missing!");
            lua_error(L);
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "Lua-error 'server.fs.is_writeable(filename): filename is not a string!");
            lua_error(L);
            return 0;
        }
        const char *filename = lua_tostring(L, 1);
        lua_pop(L, 1); // clear stack

        if (access(filename, W_OK) == 0) {
            lua_pushboolean(L, true);
        }
        else {
            lua_pushboolean(L, false);
        }
        return 1;
    }
    //=========================================================================

    /*!
     * check if a file is executable
     * LUA: server.fs.is_executable(filepath)
     */
    static int lua_fs_is_executable(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 1) {
            lua_pushstring(L, "Lua-error 'server.fs.is_executable(filename): parameter missing!");
            lua_error(L);
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "Lua-error 'server.fs.is_executable(filename): filename is not a string!");
            lua_error(L);
            return 0;
        }
        const char *filename = lua_tostring(L, 1);
        lua_pop(L, 1); // clear stack

        if (access(filename, X_OK) == 0) {
            lua_pushboolean(L, true);
        }
        else {
            lua_pushboolean(L, false);
        }
        return 1;
    }
    //=========================================================================

    /*!
     * check if a file exists
     * LUA: server.fs.exists(filepath)
     */
    static int lua_fs_exists(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 1) {
            lua_pushstring(L, "Lua-error 'server.fs.exists(filename): parameter missing!");
            lua_error(L);
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "Lua-error 'server.fs.exists(filename): filename is not a string!");
            lua_error(L);
            return 0;
        }
        const char *filename = lua_tostring(L, 1);
        lua_pop(L, 1); // clear stack

        if (access(filename, F_OK) == 0) {
            lua_pushboolean(L, true);
        }
        else {
            lua_pushboolean(L, false);
        }
        return 1;
    }
    //=========================================================================


    /*!
     * deletes a file from the file system. The file must exist and the user must have write
     * access
     * LUA: server.fs.unlink(filename)
     */
    static int lua_fs_unlink(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 1) {
            lua_pushstring(L, "Lua-error 'server.fs.unlink(filename): parameter missing!");
            lua_error(L);
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "Lua-error 'server.fs.unlink(filename): filename is not a string!");
            lua_error(L);
            return 0;
        }
        const char *filename = lua_tostring(L, 1);
        lua_pop(L, 1); // clear stack

        if (unlink(filename) != 0) {
            lua_pushstring(L, strerror(errno));
            lua_error(L);
            return 0;
        }
        return 0;
    }
    //=========================================================================

    /*!
     * Creates a new directory
     * LUA: server.fs.mkdir(dirname, tonumber('0755', 8))
     */
    static int lua_fs_mkdir(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 2) {
            lua_pushstring(L, "Lua-error 'server.fs.mkdir(dirname, mask): parameter missing!");
            lua_error(L);
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "Lua-error 'server.fs.mkdir(dirname, mask): dirname is not a string!");
            lua_error(L);
            return 0;
        }
        if (!lua_isinteger(L, 2)) {
            lua_pushstring(L, "Lua-error 'server.fs.mkdir(dirname, mask): mask is not an integer!");
            lua_error(L);
            return 0;
        }
        const char *dirname = lua_tostring(L, 1);
        int mode = lua_tointeger(L, 2);
        lua_pop(L, 2); // clear stack

        if (mkdir(dirname, mode) != 0) {
            lua_pushstring(L, strerror(errno));
            lua_error(L);
            return 0;
        }
        return 0;
    }
    //=========================================================================

    /*!
     * Creates a new directory
     * LUA: server.fs.mkdir(dirname)
     */
    static int lua_fs_rmdir(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 1) {
            lua_pushstring(L, "Lua-error 'server.fs.rmdir(dirname): parameter missing!");
            lua_error(L);
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "Lua-error 'server.fs.rmdir(dirname): dirname is not a string!");
            lua_error(L);
            return 0;
        }
        const char *dirname = lua_tostring(L, 1);
        lua_pop(L, 1); // clear stack

        if (rmdir(dirname) != 0) {
            lua_pushstring(L, strerror(errno));
            lua_error(L);
            return 0;
        }
        return 0;
    }
    //=========================================================================

    /*!
     * gets the current working directory
     * LUA: curdir = server.fs.getcwd()
     */
    static int lua_fs_getcwd(lua_State *L) {
        char *dirname = getcwd(NULL, 0);
        lua_pushstring(L, dirname);
        free(dirname);
        return 1;
    }
    //=========================================================================

    /*!
     * Change working directory
     * LUA: oldir = server.fs.chdir(newdir)
     */
    static int lua_fs_chdir(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 1) {
            lua_pushstring(L, "Lua-error 'server.fs.chdir(dirname): parameter missing!");
            lua_error(L);
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "Lua-error 'server.fs.chdir(dirname): dirname is not a string!");
            lua_error(L);
            return 0;
        }
        const char *dirname = lua_tostring(L, 1);
        lua_pop(L, 1); // clear stack

        char *olddirname = getcwd(NULL, 0);

        if (chdir(dirname) != 0) {
            lua_pushstring(L, strerror(errno));
            lua_error(L);
            return 0;
        }
        lua_pushstring(L, olddirname);
        free(olddirname);
        return 1;
    }
    //=========================================================================

    static const luaL_Reg fs_methods[] = {
            {"ftype", lua_fs_ftype},
            {"is_readable", lua_fs_is_readable},
            {"is_writeable", lua_fs_is_writeable},
            {"is_executable", lua_fs_is_executable},
            {"exists", lua_fs_exists},
            {"unlink", lua_fs_unlink},
            {"mkdir", lua_fs_mkdir},
            {"rmdir", lua_fs_rmdir},
            {"getcwd", lua_fs_getcwd},
            {"chdir", lua_fs_chdir},
            {0,     0}
    };
    //=========================================================================


    /*!
     * Generates a random version 4 uuid string
     * LUA: uuid = server.uuid()
     */
    static int lua_uuid(lua_State *L) {
        sole::uuid u4 = sole::uuid4();
        string uuidstr = u4.str();
        lua_pushstring(L, uuidstr.c_str());
        return 1;
    }
    //=========================================================================

    /*!
     * Generate a base62-uuid string
     * LUA: uuid62 = server.uuid62()
     */
    static int lua_uuid_base62(lua_State *L) {
        sole::uuid u4 = sole::uuid4();
        string uuidstr62 = u4.base62();
        lua_pushstring(L, uuidstr62.c_str());
        return 1;
    }
    //=========================================================================

    /*!
     * Converts a uuid-string to a base62 uuid
     * LUA: uuid62 = server.uuid_to_base62(uuid)
     */
    static int lua_uuid_to_base62(lua_State *L) {
        int top = lua_gettop(L);
        if (top != 1) {
            lua_pushstring(L, "Lua-error 'server.uuid_tobase62(uuid): uuid parameter missing!");
            lua_error(L);
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "Lua-error 'server.uuid_tobase62(uuid): uuid is not a string!");
            lua_error(L);
            return 0;
        }
        const char *uuidstr = lua_tostring(L, 1);
        lua_pop(L, 1); // clear stack

        sole::uuid u4 = sole::rebuild(uuidstr);
        string uuidb62str = u4.base62();
        lua_pushstring(L, uuidb62str.c_str());

        return 1;
    }
    //=========================================================================

    /*!
     * Converts a base62-uuid to a "normal" uuid
     * LUA: uuid = server.base62_to_uuid(uuid62)
     */
    static int lua_base62_to_uuid(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 1) {
            lua_pushstring(L, "Lua-error 'server.base62_to_uuid(uuid62): uuid62 parameter missing!");
            lua_error(L);
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "Lua-error 'server.base62_to_uuid(uuid62): uuid62 is not a string!");
            lua_error(L);
            return 0;
        }
        const char *uuidb62 = lua_tostring(L, 1);
        lua_pop(L, 1); // clear stack


        sole::uuid u4 = sole::rebuild(uuidb62);
        string uuidstr = u4.str();
        lua_pushstring(L, uuidstr.c_str());

        return 1;
    }
    //=========================================================================

    /*!
     * Prints variables and/or strings to the HTTP connection
     * LUA: server.print("string"|var1 [,"string|var]...)
     */
    static int lua_print(lua_State *L) {
        lua_getglobal(L, luaconnection); // push onto stack
        Connection *conn = (Connection *) lua_touserdata(L, -1); // does not change the stack
        lua_remove(L, -1); // remove from stack

        int top = lua_gettop(L);

        for (int i = 1; i <= top; i++) {
            const char *str = lua_tostring(L, i);
            if (str != NULL) {
                conn->send(str, strlen(str));
            }
        }
        lua_pop(L, top);

        return 0;
    }
    //=========================================================================


    static cJSON *subtable(lua_State *L) {
        const char table_error[] = "server.table_to_json(table): datatype inconsistency!";
        cJSON *tableobj = NULL;
        cJSON *arrayobj = NULL;
        const char *skey;
        lua_pushnil(L);  /* first key */
        while (lua_next(L, 1) != 0) {
            // key is at index -2
            // value is at index -1
            if (lua_isstring(L, -2)) {
                // we have a string as key
                skey = lua_tostring(L, -2);
                if (arrayobj != NULL) {
                    lua_pushstring(L, "server.table_to_json(table): Cannot mix int and strings as key");
                    lua_error(L);
                }
                if (tableobj == NULL) {
                    tableobj = cJSON_CreateObject();
                }
            }
            else if (lua_isinteger(L, -2)) {
                if (tableobj != NULL) {
                    lua_pushstring(L, "server.table_to_json(table): Cannot mix int and strings as key");
                    lua_error(L);
                }
                if (arrayobj == NULL) {
                    arrayobj = cJSON_CreateArray();
                }
            }
            else {
                // something else as key....
                lua_pushstring(L, "server.table_to_json(table): Cannot convert key to JSON object field");
                lua_error(L);
            }

            //
            // process values
            //
            // Check for number first, because every number will be accepted as a string.
            // The lua-functions do not check for a variable's actual type, but if they are convertable to the expected type.
            //
            if (lua_isnumber(L, -1)) {
                // a number value
                double val = lua_tonumber(L, -1);
                if (tableobj != NULL) {
                    cJSON_AddItemToObject(tableobj, skey, cJSON_CreateNumber(val));
                }
                else if (arrayobj != NULL) {
                    cJSON_AddItemToArray(arrayobj, cJSON_CreateNumber(val));
                }
                else {
                    lua_pushstring(L, table_error);
                    lua_error(L);
                }
            }
            else if (lua_isstring(L, -1)) {
                // a string value
                const char *val = lua_tostring(L, -1);
                if (tableobj != NULL) {
                    cJSON_AddItemToObject(tableobj, skey, cJSON_CreateString(val));
                }
                else if (arrayobj != NULL) {
                    cJSON_AddItemToArray(arrayobj, cJSON_CreateString(val));
                }
                else {
                    lua_pushstring(L, table_error);
                    lua_error(L);
                }
            }
            else if (lua_isboolean(L, -1)) {
                // a boolean value
                bool val = lua_toboolean(L, -1);
                if (tableobj != NULL) {
                    cJSON_AddItemToObject(tableobj, skey, cJSON_CreateBool(val));
                }
                else if (arrayobj != NULL) {
                    cJSON_AddItemToArray(arrayobj, cJSON_CreateBool(val));
                }
                else {
                    lua_pushstring(L, table_error);
                    lua_error(L);
                }
            }
            else if (lua_istable(L, -1)) {
                if (tableobj != NULL) {
                    cJSON_AddItemToObject(tableobj, skey, subtable(L));
                }
                else if (arrayobj != NULL) {
                    cJSON_AddItemToArray(arrayobj, subtable(L));
                }
                else {
                    lua_pushstring(L, table_error);
                    lua_error(L);
                }
            }
            else {
                lua_pushstring(L, table_error);
                lua_error(L);
            }
            lua_pop(L, 1);
        }
        return tableobj;
    }
    //=========================================================================

    static int lua_table_to_json(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 1) {
            lua_pushstring(L, "Lua-error 'server.table_to_json(table): table parameter missing!");
            lua_error(L);
            return 0;
        }
        if (!lua_istable(L, 1)) {
            lua_pushstring(L, "Lua-error 'server.table_to_json(table): table is not a lua-table!");
            lua_error(L);
            return 0;
        }

        cJSON *root = subtable(L);
        char *jsonstr = cJSON_Print(root);
        lua_pushstring(L, jsonstr);
        free(jsonstr);
        return 1;
    }
    //=========================================================================

    /*!
     * Stops the HTTP server and exits everything
     * TODO: There must be some authorization control here. This function should only be available for admins
     * TODO: Add the possibility to give a message which is printed on the console and logfiles
     * LUA: server.shutdown()
     */
    static int lua_exitserver(lua_State *L) {
        lua_getglobal(L, luaconnection); // push onto stack
        Connection *conn = (Connection *) lua_touserdata(L, -1); // does not change the stack
        lua_remove(L, -1); // remove from stack

        conn->server()->stop();

        return 0;
    }
    //=========================================================================

    /*!
     * Adds a new HTTP header field
     * LUA: server.sendHeader(key, value)
     */
    static int lua_header(lua_State *L) {
        lua_getglobal(L, luaconnection);
        Connection *conn = (Connection *) lua_touserdata(L, -1);
        lua_remove(L, -1); // remove from stack

        int top = lua_gettop(L);

        if (top != 2) {
            lua_pushstring(L, "Lua-error 'server.header(key,val): Invalid number of parameters!");
            lua_error(L);
            return 0;
        }
        const char *hkey = lua_tostring(L, 1);
        const char *hval = lua_tostring(L, 2);

        conn->header(hkey, hval);

        return 0;
    }
    //=========================================================================

    /*!
     * Copy an uploaded file to another location
     *
     * shttp saves uploaded files in a temporary location (given by the config variable "tmpdir")
     * and deletes it after the request has been served. This function is used to copy the file
     * to another location where it can be used/retrieved by shttps/sipi.
     *
     * LUA: server.copyTmpfile()
     *
     */
    static int lua_copytmpfile(lua_State *L) {
        lua_getglobal(L, luaconnection);
        Connection *conn = (Connection *) lua_touserdata(L, -1);
        lua_remove(L, -1); // remove from stack

        int top = lua_gettop(L);
        int tmpfile_id = lua_tointeger(L, 1);
        const char *outfile = lua_tostring(L, 2);

        vector <Connection::UploadedFile> uploads = conn->uploads();

        string infile = uploads[tmpfile_id - 1].tmpname;
        ifstream source(infile, ios::binary);
        ofstream dest(outfile, ios::binary);

        dest << source.rdbuf();

        source.close();
        dest.close();


        return 0;
    }
    //=========================================================================

    /*!
     * This function registers all variables and functions in the server table
     */
    void LuaServer::createGlobals(Connection &conn) {
        lua_createtable(L, 0, 13); // table1

        Connection::HttpMethod method = conn.method();
        lua_pushstring(L, "method"); // table1 - "index_L1"
        switch (method) {
            case Connection::OPTIONS:
                lua_pushstring(L, "OPTIONS");
                break; // table1 - "index_L1" - "value_L1"
            case Connection::GET:
                lua_pushstring(L, "GET");
                break;
            case Connection::HEAD:
                lua_pushstring(L, "HEAD");
                break;
            case Connection::POST:
                lua_pushstring(L, "POST");
                break;
            case Connection::PUT:
                lua_pushstring(L, "PUT");
                break;
            case Connection::DELETE:
                lua_pushstring(L, "DELETE");
                break;
            case Connection::TRACE:
                lua_pushstring(L, "TRACE");
                break;
            case Connection::CONNECT:
                lua_pushstring(L, "CONNECT");
                break;
            case Connection::OTHER:
                lua_pushstring(L, "OTHER");
                break;
        }
        lua_rawset(L, -3); // table1

        lua_pushstring(L, "header"); // table1 - "index_L1"
        std::vector<std::string> headers = conn.header();
        lua_createtable(L, 0, headers.size()); // table1 - "index_L1" - table2
        for (unsigned i = 0; i < headers.size(); i++) {
            lua_pushstring(L, headers[i].c_str()); // table1 - "index_L1" - table2 - "index_L2"
            lua_pushstring(L,
                           conn.header(headers[i]).c_str()); // table1 - "index_L1" - table2 - "index_L2" - "value_L2"
            lua_rawset(L, -3); // table1 - "index_L1" - table2
        }
        lua_rawset(L, -3); // table1

        string host = conn.host();
        lua_pushstring(L, "host"); // table1 - "index_L1"
        lua_pushstring(L, host.c_str()); // table1 - "index_L1" - "value_L1"
        lua_rawset(L, -3); // table1

        string uri = conn.uri();
        lua_pushstring(L, "uri"); // table1 - "index_L1"
        lua_pushstring(L, uri.c_str()); // table1 - "index_L1" - "value_L1"
        lua_rawset(L, -3); // table1

        lua_pushstring(L, "get"); // table1 - "index_L1"
        std::vector<std::string> get_params = conn.getParams();
        lua_createtable(L, 0, get_params.size()); // table1 - "index_L1" - table2
        for (unsigned i = 0; i < get_params.size(); i++) {
            lua_pushstring(L, get_params[i].c_str()); // table1 - "index_L1" - table2 - "index_L2"
            lua_pushstring(L, conn.getParams(
                    get_params[i]).c_str()); // table1 - "index_L1" - table2 - "index_L2" - "value_L2"
            lua_rawset(L, -3); // table1 - "index_L1" - table2
        }
        lua_rawset(L, -3); // table1

        lua_pushstring(L, "post"); // table1 - "index_L1"
        std::vector<std::string> post_params = conn.postParams();
        lua_createtable(L, 0, post_params.size()); // table1 - "index_L1" - table2
        for (unsigned i = 0; i < post_params.size(); i++) {
            lua_pushstring(L, post_params[i].c_str()); // table1 - "index_L1" - table2 - "index_L2"
            lua_pushstring(L, conn.postParams(
                    post_params[i]).c_str()); // table1 - "index_L1" - table2 - "index_L2" - "value_L2"
            lua_rawset(L, -3); // table1 - "index_L1" - table2
        }
        lua_rawset(L, -3); // table1

        vector <Connection::UploadedFile> uploads = conn.uploads();
        lua_pushstring(L, "uploads"); // table1 - "index_L1"
        lua_createtable(L, 0, uploads.size());     // table1 - "index_L1" - table2
        for (unsigned i = 0; i < uploads.size(); i++) {
            lua_pushinteger(L, i + 1);             // table1 - "index_L1" - table2 - "index_L2"
            lua_createtable(L, 0, uploads.size()); // "table1" - "index_L1" - "table2" - "index_L2" - "table3"

            lua_pushstring(L,
                           "fieldname");          // "table1" - "index_L1" - "table2" - "index_L2" - "table3" - "index_L3"
            lua_pushstring(L,
                           uploads[i].fieldname.c_str()); // "table1" - "index_L1" - "table2" - "index_L2" - "table3" - "index_L3" - "value_L3"
            lua_rawset(L, -3);                       // "table1" - "index_L1" - "table2" - "index_L2" - "table3"

            lua_pushstring(L,
                           "origname");          // "table1" - "index_L1" - "table2" - "index_L2" - "table3" - "index_L3"
            lua_pushstring(L,
                           uploads[i].origname.c_str()); // "table1" - "index_L1" - "table2" - "index_L2" - "table3" - "index_L3" - "value_L3"
            lua_rawset(L, -3);                      // "table1" - "index_L1" - "table2" - "index_L2" - "table3"

            lua_pushstring(L,
                           "tmpname");          // "table1" - "index_L1" - "table2" - "index_L2" - "table3" - "index_L3"
            lua_pushstring(L,
                           uploads[i].tmpname.c_str()); // "table1" - "index_L1" - "table2" - "index_L2" - "table3" - "index_L3" - "value_L3"
            lua_rawset(L, -3);                     // "table1" - "index_L1" - "table2" - "index_L2" - "table3"

            lua_pushstring(L,
                           "mimetype");          // "table1" - "index_L1" - "table2" - "index_L2" - "table3" - "index_L3"
            lua_pushstring(L,
                           uploads[i].mimetype.c_str()); // "table1" - "index_L1" - "table2" - "index_L2" - "table3" - "index_L3" - "value_L3"
            lua_rawset(L, -3);                      // "table1" - "index_L1" - "table2" - "index_L2" - "table3"

            lua_pushstring(L,
                           "filesize");           // "table1" - "index_L1" - "table2" - "index_L2" - "table3" - "index_L3"
            lua_pushinteger(L,
                            uploads[i].filesize); // "table1" - "index_L1" - "table2" - "index_L2" - "table3" - "index_L3" - "value_L3"
            lua_rawset(L, -3);                       // "table1" - "index_L1" - "table2" - "index_L2" - "table3"

            lua_rawset(L, -3); // table1 - "index_L1" - table2
        }
        lua_rawset(L, -3); // table1

        lua_pushstring(L, "request"); // table1 - "index_L1"
        std::vector<std::string> request_params = conn.requestParams();
        lua_createtable(L, 0, request_params.size()); // table1 - "index_L1" - table2
        for (unsigned i = 0; i < request_params.size(); i++) {
            lua_pushstring(L, request_params[i].c_str()); // table1 - "index_L1" - table2 - "index_L2"
            lua_pushstring(L, conn.requestParams(
                    request_params[i]).c_str()); // table1 - "index_L1" - table2 - "index_L2" - "value_L2"
            lua_rawset(L, -3); // table1 - "index_L1" - table2
        }
        lua_rawset(L, -3); // table1

        //
        // filesystem functions
        //
        lua_pushstring(L, "fs"); // table1 - "fs"
        lua_newtable(L); // table1 - "fs" - table2
        luaL_setfuncs(L, fs_methods, 0);
        lua_rawset(L, -3); // table1

        //
        // jsonstr = server.table_to_json(table)
        //
        lua_pushstring(L, "table_to_json"); // table1 - "index_L1"
        lua_pushcfunction(L, lua_table_to_json); // table1 - "index_L1" - function
        lua_rawset(L, -3); // table1

        //
        // server.print (var[, var...])
        //
        lua_pushstring(L, "print"); // table1 - "index_L1"
        lua_pushcfunction(L, lua_print); // table1 - "index_L1" - function
        lua_rawset(L, -3); // table1

        lua_pushstring(L, "uuid"); // table1 - "index_L1"
        lua_pushcfunction(L, lua_uuid); // table1 - "index_L1" - function
        lua_rawset(L, -3); // table1

        lua_pushstring(L, "uuid62"); // table1 - "index_L1"
        lua_pushcfunction(L, lua_uuid_base62); // table1 - "index_L1" - function
        lua_rawset(L, -3); // table1

        lua_pushstring(L, "uuid_to_base62"); // table1 - "index_L1"
        lua_pushcfunction(L, lua_uuid_to_base62); // table1 - "index_L1" - function
        lua_rawset(L, -3); // table1

        lua_pushstring(L, "base62_to_uuid"); // table1 - "index_L1"
        lua_pushcfunction(L, lua_base62_to_uuid); // table1 - "index_L1" - function
        lua_rawset(L, -3); // table1

        lua_pushstring(L, "setBuffer"); // table1 - "index_L1"
        lua_pushcfunction(L, lua_setbuffer); // table1 - "index_L1" - function
        lua_rawset(L, -3); // table1


        lua_pushstring(L, "sendHeader"); // table1 - "index_L1"
        lua_pushcfunction(L, lua_header); // table1 - "index_L1" - function
        lua_rawset(L, -3); // table1

        lua_pushstring(L, "copyTmpfile"); // table1 - "index_L1"
        lua_pushcfunction(L, lua_copytmpfile); // table1 - "index_L1" - function
        lua_rawset(L, -3); // table1

        lua_pushstring(L, "shutdown"); // table1 - "index_L1"
        lua_pushcfunction(L, lua_exitserver); // table1 - "index_L1" - function
        lua_rawset(L, -3); // table1

        lua_setglobal(L, servertablename);

        lua_pushlightuserdata(L, &conn);
        lua_setglobal(L, luaconnection);
    }
    //=========================================================================


    void LuaServer::add_servertableentry(const string &name, const string &value) {
        lua_getglobal(L, servertablename); // "table1"

        lua_pushstring(L, name.c_str()); // table1 - "index_L1"
        lua_pushstring(L, value.c_str()); // table1 - "index_L1" - "value_L1"
        lua_rawset(L, -3); // table1
        lua_pop(L, 1);
    }
    //=========================================================================



    string LuaServer::configString(const string table, const string variable, const string defval) {
        lua_getglobal(L, table.c_str());
        lua_getfield(L, -1, variable.c_str());
        if (lua_isnil(L, -1)) {
            lua_pop(L, 2);
            return defval;
        }
        if (!lua_isstring(L, -1)) {
            throw Error(__file__, __LINE__, "String expected for " + table + "." + variable);
        }
        string retval = lua_tostring(L, -1);
        lua_pop(L, 2);
        return retval;
    }
    //=========================================================================


    int LuaServer::configBoolean(const string table, const string variable, const bool defval) {
        lua_getglobal(L, table.c_str());
        lua_getfield(L, -1, variable.c_str());
        if (lua_isnil(L, -1)) {
            lua_pop(L, 2);
            return defval;
        }
        if (!lua_isboolean(L, -1)) {
            throw Error(__file__, __LINE__, "Integer expected for " + table + "." + variable);
        }
        bool retval = lua_toboolean(L, -1);

        lua_pop(L, 2);
        return retval;
    }
    //=========================================================================

    int LuaServer::configInteger(const string table, const string variable, const int defval) {
        lua_getglobal(L, table.c_str());
        lua_getfield(L, -1, variable.c_str());
        if (lua_isnil(L, -1)) {
            lua_pop(L, 2);
            return defval;
        }
        if (!lua_isinteger(L, -1)) {
            throw Error(__file__, __LINE__, "Integer expected for " + table + "." + variable);
        }
        int retval = lua_tointeger(L, -1);
        lua_pop(L, 2);
        return retval;
    }
    //=========================================================================

    float LuaServer::configFloat(const string table, const string variable, const float defval) {
        lua_getglobal(L, table.c_str());
        lua_getfield(L, -1, variable.c_str());
        if (lua_isnil(L, -1)) {
            lua_pop(L, 2);
            return defval;
        }
        if (!lua_isnumber(L, -1)) {
            throw Error(__file__, __LINE__, "Number expected for " + table + "." + variable);
        }
        lua_Number num = lua_tonumber(L, -1);
        lua_pop(L, 2);
        return (float) num;
    }
    //=========================================================================


    const vector <LuaRoute> LuaServer::configRoute(const string routetable) {
        static struct {
            const char *name;
            int type;
        } fields[] = {
                {"method", LUA_TSTRING},
                {"route",  LUA_TSTRING},
                {"script", LUA_TSTRING},
                {NULL,     0}
        };

        vector <LuaRoute> routes;

        lua_getglobal(L, routetable.c_str());
        luaL_checktype(L, -1, LUA_TTABLE);
        int i;
        for (i = 1; ; i++) {
            lua_rawgeti(L, -1, i);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                break;
            }
            // an element of the 'listen' table should now be at the top of the stack
            luaL_checktype(L, -1, LUA_TTABLE);
            int field_index;
            LuaRoute route;
            for (field_index = 0; (fields[field_index].name != NULL
                                   && fields[field_index].name != NULL); field_index++) {
                lua_getfield(L, -1, fields[field_index].name);
                luaL_checktype(L, -1, fields[field_index].type);

                string method;
                // you should probably use a function pointer in the fields table.
                // I am using a simple switch/case here
                switch (field_index) {
                    case 0:
                        method = lua_tostring(L, -1);
                        if (method == "GET") {
                            route.method = Connection::HttpMethod::GET;
                        }
                        else if (method == "PUT") {
                            route.method = Connection::HttpMethod::PUT;
                        }
                        else if (method == "POST") {
                            route.method = Connection::HttpMethod::POST;
                        }
                        else if (method == "DELETE") {
                            route.method = Connection::HttpMethod::DELETE;
                        }
                        else if (method == "OPTIONS") {
                            route.method = Connection::HttpMethod::OPTIONS;
                        }
                        else if (method == "CONNECT") {
                            route.method = Connection::HttpMethod::CONNECT;
                        }
                        else if (method == "HEAD") {
                            route.method = Connection::HttpMethod::HEAD;
                        }
                        else if (method == "OTHER") {
                            route.method = Connection::HttpMethod::OTHER;
                        }
                        else {
                            throw Error(__file__, __LINE__, "Unknown HTTP method!");
                        }
                        break;
                    case 1:
                        route.route = lua_tostring(L, -1);
                        break;
                    case 2:
                        route.script = lua_tostring(L, -1);
                        break;
                }
                // remove the field value from the top of the stack
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
            routes.push_back(route);
        }
        return routes;
    }
    //=========================================================================


    void LuaServer::executeChunk(const string &luastr) {
        if (luaL_dostring(L, luastr.c_str()) != 0) {
            const char *luaerror = lua_tostring(L, -1);
            throw Error(__file__, __LINE__, string("Lua error: ") + luaerror);
        }
    }
    //=========================================================================


    vector <LuaValstruct> LuaServer::executeLuafunction(const string *funcname, int n, LuaValstruct *lv) {
        if (lua_getglobal(L, funcname->c_str()) != LUA_TFUNCTION) {
            lua_pop(L, 1);
            throw Error(__file__, __LINE__, "Function not existing!");
        }
        for (int i = 0; i < n; i++) {
            switch (lv[i].type) {
                case LuaValstruct::INT_TYPE: {
                    lua_pushinteger(L, lv[i].value.i);
                    break;
                }
                case LuaValstruct::FLOAT_TYPE: {
                    lua_pushnumber(L, lv[i].value.f);
                    break;
                }
                case LuaValstruct::STRING_TYPE: {
                    lua_pushstring(L, lv[i].value.s.c_str());
                    break;
                }
            }

        }

        lua_call(L, n, LUA_MULTRET);

        int top = lua_gettop(L);
        vector <LuaValstruct> retval;

        LuaValstruct tmplv;
        for (int i = 1; i <= top; i++) {
            if (lua_isstring(L, i)) {
                tmplv.type = LuaValstruct::STRING_TYPE;
                tmplv.value.s = string(lua_tostring(L, i));
            }
            else if (lua_isinteger(L, i)) {
                tmplv.type = LuaValstruct::INT_TYPE;
                tmplv.value.i = lua_tointeger(L, i);
            }
            else if (lua_isnumber(L, i)) {
                tmplv.type = LuaValstruct::FLOAT_TYPE;
                tmplv.value.f = (float) lua_tonumber(L, i);
            }
            else {
                throw Error(__file__, __LINE__, "Datatype cannot be returned!");
            }
            retval.push_back(tmplv);
        }

        lua_pop(L, top);

        return retval;
    }
    //=========================================================================


    vector <LuaValstruct> LuaServer::executeLuafunction(const string *funcname, int n, ...) {
        va_list args;
        va_start(args, n);

        if (lua_getglobal(L, funcname->c_str()) != LUA_TFUNCTION) {
            lua_pop(L, 1);
            throw Error(__file__, __LINE__, "Function not existing!");
        }

        for (int i = 0; i < n; i++) {
            LuaValstruct *lv = va_arg(args, LuaValstruct*);
            switch (lv->type) {
                case LuaValstruct::INT_TYPE: {
                    lua_pushinteger(L, lv->value.i);
                    break;
                }
                case LuaValstruct::FLOAT_TYPE: {
                    lua_pushnumber(L, lv->value.f);
                    break;
                }
                case LuaValstruct::STRING_TYPE: {
                    lua_pushstring(L, lv->value.s.c_str());
                    break;
                }
            }

        }
        va_end(args);

        lua_call(L, n, 1);

        int top = lua_gettop(L);
        vector <LuaValstruct> retval;

        LuaValstruct tmplv;
        for (int i = 1; i <= top; i++) {
            if (lua_isstring(L, i)) {
                tmplv.type = LuaValstruct::STRING_TYPE;
                tmplv.value.s = string(lua_tostring(L, i));
            }
            else if (lua_isinteger(L, i)) {
                tmplv.type = LuaValstruct::INT_TYPE;
                tmplv.value.i = lua_tointeger(L, i);
            }
            else if (lua_isnumber(L, i)) {
                tmplv.type = LuaValstruct::FLOAT_TYPE;
                tmplv.value.f = (float) lua_tonumber(L, i);
            }
            else {
                throw Error(__file__, __LINE__, "Datatype cannot be returned!");
            }
            retval.push_back(tmplv);
        }

        lua_pop(L, top);

        return retval;
    }
    //=========================================================================

    bool LuaServer::luaFunctionExists(const std::string *funcname) {
        int ltype = lua_getglobal(L, funcname->c_str());
        lua_pop(L, 1);
        return (ltype == LUA_TFUNCTION);
    }
}
