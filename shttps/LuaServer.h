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
 *//*!
 * \brief Lua handling...
 *
 */
#ifndef __shttp_lua_server_h
#define __shttp_lua_server_h

#include <iostream>
#include <vector>
#include <map>

#include "Error.h"
#include "Connection.h"

#include "lua.hpp"


namespace shttps {

    typedef struct {
        enum {INT_TYPE, FLOAT_TYPE, STRING_TYPE} type;
        struct {
            int i;
            float f;
            std::string s;
        } value;
        //inline LuaValstruct() { type = }
    } LuaValstruct;

    typedef struct _LuaRoute {
        Connection::HttpMethod method;
        std::string route;
        std::string script;
    } LuaRoute;

    typedef void (*LuaSetGlobalsFunc)(lua_State *L, Connection &, void *);

    extern char luaconnection[];

    class LuaServer {
    private:
        lua_State *L;
        //std::vector<LuaSetGlobalsFunc> setGlobals;
    public:
       /*!
        * Instantiates a lua interpreter
        */
        LuaServer();

       /*!
        * Instantiates a lua interpreter which has access to the HTTP connection
        *
        * \param[in] conn HTTP Connection object
        */
        LuaServer(Connection &conn);


       /*!
        * Instantiates a lua interpreter an executes the given lua script
        *
        * \param[in] luafile A script containing lua commands
        * \param[in] iscode If true, the string contains lua-code to be executed directly
        */
        LuaServer(const std::string &luafile, bool iscode = false);

       /*!
        * Instantiates a lua interpreter an executes the given lua script
        *
        * \param[in] conn HTTP Connection object
        * \param[in] luafile A script containing lua commands
        * \param[in] iscode If true, the string contains lua-code to be executed directly
        */
        LuaServer(Connection &conn, const std::string &luafile, bool iscode);

       /*!
        * Copy constructor throws error (not allowed!)
        */
        inline LuaServer(const LuaServer &other) {
            throw Error(__FILE__, __LINE__, "Copy constructor not allowed!");
        }

       /*!
        * Assignment operator throws error (not allowed!)
        */
        inline LuaServer& operator=(const LuaServer& other) {
            throw Error(__FILE__, __LINE__, "Assigment operator not allowed!");
        }

       /*!
        * Destroys the lua interpreter and all associated resources
        */
        ~LuaServer();

       /*!
        * Getter for the Lua state
        */
        inline lua_State *lua() { return L; }

       /*!
        * Adds a value to the server tabe.
        *
        * The server table contains constants and functions which are related
        * to the shttps HTTP server. The server table a a globally accessible
        * table.
        *
        * \param[in] name Name of variable
        * \param[in] value Value (only String allowed)
        */
        void add_servertableentry(const std::string &name, const std::string &value);

       /*!
        * Create the global values and functions
        *
        * \param[in] conn HTTP connection object
        */
        void createGlobals(Connection &conn);


        std::string configString(const std::string table, const std::string variable, const std::string defval);
        int configBoolean(const std::string table, const std::string variable, const bool defval);
        int configInteger(const std::string table, const std::string variable, const int defval);
        float configFloat(const std::string table, const std::string variable, const float defval);
        const std::vector<LuaRoute> configRoute(const std::string routetable);

       /*!
        * Execute a chunk of Lua code
        *
        * \param[in] luastr String containing the Lua code
        * \returns Either the value 1 or an integer result that the Lua code provides
        */
        int executeChunk(const std::string &luastr);

       /*!
        * Executes a Lua function that either is defined in C or in Lua
        *
        * \param[in] funcname Name of the function
        * \param[in] n Number of arguments
        * \returns vector of LuaValstruct containing the result of the execution of the lua function
        */
        std::vector<LuaValstruct>  executeLuafunction(const std::string *funcname, int n, ...);

        /*!
         * Executes a Lua function that either is defined in C or in Lua
         *
         * \param[in] funcname Name of the function
         * \param[in] n Number of arguments
         * \param[in] lv Array of LuaValstruct values
         * \returns vector of LuaValstruct containing the result of the execution of the lua function
         */
        std::vector<LuaValstruct>  executeLuafunction(const std::string *funcname, int n, LuaValstruct *lv);

        /*!
         * Executes a Lua function that either is defined in C or in Lua
         *
         * \param[in] funcname Name of the function
         * \param[in] n Number of arguments
         * \returns true if function with given name exists
         */
        bool luaFunctionExists(const std::string *funcname);
    };

}

#endif
