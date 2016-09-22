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
#include <string>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <stdio.h>
#include <sqlite3.h>
#include "LuaSqlite.h"

#include "SipiHttpServer.h"

using namespace std;

namespace shttps {


    char shttpsserver[] = "__shttps";

    static const char LUASQLITE[] = "Sqlite";

    typedef struct {
        sqlite3 *sqlite_handle;
        std::string *dbname;
    } Sqlite;

    static Sqlite *toSqlite(lua_State *L, int index) {
        Sqlite *db = (Sqlite *) lua_touserdata(L, index);
        if (db == NULL) {
            lua_pushstring(L, "Type error");
            lua_error(L);
        }
        return db;
    }
    //=========================================================================

    static Sqlite *checkSqlite(lua_State *L, int index) {
        Sqlite *db;
        luaL_checktype(L, index, LUA_TUSERDATA);
        db = (Sqlite *) luaL_checkudata(L, index, LUASQLITE);
        if (db == NULL) {
            lua_pushstring(L, "Type error");
            lua_error(L);
        }
        return db;
    }
    //=========================================================================

    static Sqlite *pushSqlite(lua_State *L) {
        Sqlite *db = (Sqlite *) lua_newuserdata(L, sizeof(Sqlite));
        luaL_getmetatable(L, LUASQLITE);
        lua_setmetatable(L, -2);
        return db;
    }
    //=========================================================================

    //
    // called by garbage collection
    //
    static int Sqlite_gc(lua_State *L) {
        Sqlite *db = toSqlite(L, 1);
        if (db->sqlite_handle != NULL) sqlite3_close_v2(db->sqlite_handle);
        delete db->dbname;
        return 0;
    }
    //=========================================================================

    static int Sqlite_tostring(lua_State *L) {
        Sqlite *db = toSqlite(L, 1);
        std::stringstream ss;
        ss << "DB-File: " << *(db->dbname);
        lua_pushstring(L, ss.str().c_str());
        return 1;
    }
    //=========================================================================

    static int sqlite_query(lua_State *L);

    static const luaL_Reg Sqlite_meta[] = {
            {"__gc",       Sqlite_gc},
            {"__tostring", Sqlite_tostring},
            {"__shl",      sqlite_query},
            {0,            0}
    };
    //=========================================================================



    static const char LUASQLSTMT[] = "Stmt";

    typedef struct {
        sqlite3 *sqlite_handle;
        sqlite3_stmt *stmt_handle;
    } Stmt;

    static Stmt *toStmt(lua_State *L, int index) {
        Stmt *stmt = (Stmt *) lua_touserdata(L, index);
        if (stmt == NULL) {
            lua_pushstring(L, "Type error");
            lua_error(L);
        }
        return stmt;
    }
    //=========================================================================

    static Stmt *checkStmt(lua_State *L, int index) {
        luaL_checktype(L, index, LUA_TUSERDATA);

        Stmt *stmt = (Stmt *) luaL_checkudata(L, index, LUASQLSTMT);
        if (stmt == NULL) {
            lua_pushstring(L, "Type error");
            lua_error(L);
        }
        return stmt;
    }
    //=========================================================================

    static Stmt *pushStmt(lua_State *L) {
        Stmt *stmt = (Stmt *) lua_newuserdata(L, sizeof(Stmt));
        luaL_getmetatable(L, LUASQLSTMT);
        lua_setmetatable(L, -2);
        return stmt;
    }
    //=========================================================================

    //
    // called by garbage collection
    //
    static int Stmt_gc(lua_State *L) {
        Stmt *stmt = toStmt(L, 1);
        if (stmt->stmt_handle != NULL) sqlite3_finalize(stmt->stmt_handle);
        return 0;
    }
    //=========================================================================

    static int Stmt_tostring(lua_State *L) {
        Stmt *stmt = toStmt(L, 1);
        std::stringstream ss;
        ss << "SQL: " << sqlite3_sql(stmt->stmt_handle);
        lua_pushstring(L, ss.str().c_str());
        return 1;
    }
    //=========================================================================

    static const luaL_Reg Stmt_meta[] = {
            {"__gc",       Stmt_gc},
            {"__tostring", Stmt_tostring},
            {0,            0}
    };
    //=========================================================================



    //
    // usage:
    //
    //   Sqlite.new(path [, "RO" | "RW" | "CRW"])
    static int sqlite_new(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 1) {
            // throw an error!
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            // thow an error!
            return 0;
        }
        const char *dbpath = lua_tostring(L, 1);

        int flags = SQLITE_OPEN_READWRITE;
        if ((top >= 2) && (lua_isstring(L,2))) {
            string flagstr = lua_tostring(L, 1);
            if (flagstr == "RO") {
                flags = SQLITE_OPEN_READONLY;
            }
            else if (flagstr == "RW") {
                flags = SQLITE_OPEN_READWRITE;
            }
            else if (flagstr == "CRW") {
                flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
            }
        }
        flags |=  SQLITE_OPEN_NOMUTEX;

        sqlite3 *handle;
        int status = sqlite3_open_v2(dbpath, &handle, flags, NULL);
        if (status !=  SQLITE_OK) {
            lua_pushstring(L, sqlite3_errmsg(handle));
            lua_error(L);
            return 0;
        }

        Sqlite *db = pushSqlite(L);
        db->sqlite_handle = handle;
        db->dbname = new std::string(dbpath);

        return 1;
    }
    //=========================================================================

    //
    // usage
    //    db = sqlite.new("/path/to/db/file", "RW")
    //    qry = sqlite.query(db, "SELECT * FROM test")
    //    -- or --
    //    qry = db << "SELECT * FROM test"
    //
    static int sqlite_query(lua_State *L) {
        int top = lua_gettop(L);
        if (top != 2) {
            // throw an error!
            lua_pushstring(L, "Incorrect number of arguments!");
            lua_error(L);
            return 0;
        }
        Sqlite *db = checkSqlite(L, 1);
        if (db == NULL) {
            lua_pushstring(L, "Couldn't connect to database!");
            lua_error(L);
            return 0;
        }
        const char *sql = NULL;
        if (lua_isstring(L, 2)) {
            sql = lua_tostring(L, 2);
        }
        else {
            lua_pushstring(L, "No SQL given!");
            lua_error(L);
            return 0;
        }
        sqlite3_stmt *stmt_handle;
        int status = sqlite3_prepare_v2(db->sqlite_handle, sql, strlen(sql), &stmt_handle, NULL);
        if (status !=  SQLITE_OK) {
            lua_pushstring(L, sqlite3_errmsg(db->sqlite_handle));
            lua_error(L);
            return 0;
        }

        Stmt *stmt = pushStmt(L);
        stmt->sqlite_handle = db->sqlite_handle; // we save the handle of the database also here
        stmt->stmt_handle = stmt_handle;
        return 1;
    }
    //=========================================================================

    //
    // usage
    //
    //  db = sqlite.new("/path/to/db/file", "RW")
    //  qry = sqlite.query(db, "SELECT * FROM test")
    //  row = sqlite.next(qry)
    //  while (row) do
    //      ...
    //      row = sqlite.next(db, res)
    //  end
    //
    // -- or --
    //
    // db = sqlite.new("/path/to/db/file", "RW")
    // qry = db << "SELECT * FROM test"
    // row = qry()
    // while (row) do
    //    ...
    //    row = qry()
    // end
    //
    int sqlite_next(lua_State *L) {
        int top = lua_gettop(L);

        if (top != 1) {
            // throw an error!
            lua_pushstring(L, "Incorrect number of arguments!");
            lua_error(L);
            return 0;
        }

        Stmt *stmt = checkStmt(L, 1);
        if (stmt == NULL) {
            lua_pushstring(L, "Invalid prepared statment!");
            lua_error(L);
            return 0;
        }

        int status = sqlite3_step(stmt->stmt_handle);
        if (status ==  SQLITE_ROW) {
            int ncols = sqlite3_column_count(stmt->stmt_handle);

            lua_createtable(L, 0, ncols); // table

            for (int col = 0; col < ncols; col++) {
                lua_pushinteger(L, col); // table - col
                int ctype =  sqlite3_column_type(stmt->stmt_handle, col);
                switch (ctype) {
                    case SQLITE_INTEGER: {
                        int val = sqlite3_column_int(stmt->stmt_handle, col);
                        lua_pushinteger(L, val); // table - col - val
                        break;
                    }
                    case SQLITE_FLOAT: {
                        double val = sqlite3_column_double(stmt->stmt_handle, col);
                        lua_pushnumber(L, val); // table - col - val
                        break;
                    }
                    case SQLITE_BLOB: {
                        size_t nval = sqlite3_column_bytes(stmt->stmt_handle, col);
                        const char *val = (char *) sqlite3_column_blob(stmt->stmt_handle, col);
                        lua_pushlstring(L, val, nval);
                        break;
                    }
                    case SQLITE_NULL: {
                        lua_pushnil(L); // table - col - nil
                        break;
                    }
                    case SQLITE_TEXT: {
                        const char *str = (char *) sqlite3_column_text(stmt->stmt_handle, col);
                        lua_pushstring(L, str); // table - col - str
                        break;
                    }
                }
                lua_rawset(L, -3); // table
            }
        }
        else if (status == SQLITE_DONE) {
            lua_pushnil(L);
        }
        else {
            lua_pushstring(L, sqlite3_errmsg(stmt->sqlite_handle));
            lua_error(L);
            return 0;
        }
        return 1;
    }
    //=========================================================================

    // map the method names exposed to Lua to the names defined here
    static const luaL_Reg sqlite_methods[] = {
            {"new", sqlite_new},
            {"query", sqlite_query},
            {"next", sqlite_next},
            {0,     0}
    };
    //=========================================================================


    void sqliteGlobals(lua_State *L, shttps::Connection &conn, void *user_data) {
        //
        // let's prepare the metatable for the stmt-object
        //

        luaL_newmetatable(L, LUASQLSTMT); // create metatable, and add it to the Lua registry
        // stack: metatable


        luaL_setfuncs(L, Stmt_meta, 0);
        // stack: metatable  [with functions __gc and __tostring added]

        lua_pop(L, 1);

        //
        // now let's create the Sqlite object. It's a table...
        //
        lua_getglobal(L, LUASQLITE);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
        }
        // stack:  table(LUASQLITE)

        //
        // now we add the function elements to the table
        //
        luaL_setfuncs(L, sqlite_methods, 0);
        // stack:  table(LUASQLITE)

        luaL_newmetatable(L, LUASQLITE); // create metatable, and add it to the Lua registry
        // stack: table(LUASQLITE) - metatable

        luaL_setfuncs(L, Sqlite_meta, 0);

        lua_pushliteral(L, "__index");
        // stack: table(LUASQLITE) - metatable - "__index"

        lua_pushvalue(L, -3); // dup methods table
        // stack: table(LUASQLITE) - metatable - "__index" - table(LUASQLITE)

        lua_rawset(L, -3); // metatable.__index = methods
        // stack: table(LUASQLITE) - metatable

        lua_pushliteral(L, "__metatable");
        // stack: table(LUASQLITE) - metatable - "__metatable"

        lua_pushvalue(L, -3); // dup methods table
        // stack: table(LUASQLITE) - metatable - "__metatable" - table(LUASQLITE)

        lua_rawset(L, -3);
        // stack: table(LUASQLITE) - metatable

        lua_pop(L, 1); // drop metatable
        // stack: table(LUASQLITE)

        lua_setglobal(L, LUASQLITE);
        // stack: empty

    }
    //=========================================================================


} // namespace
