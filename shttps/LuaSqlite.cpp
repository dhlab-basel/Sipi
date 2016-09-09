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
#include "LuaSqlite.h"

#include "SipiHttpServer.h"

use namespace std;

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
        return img;
    }
    //=========================================================================

    //
    // called by garbage collection
    //
    static int Sqlite_gc(lua_State *L) {
        Sqlite *db = toSqlite(L, 1);
        sqlite3_close_v2(db->sqlite_handle);
        delete db->dbame;
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

    static const luaL_Reg Sqlite_meta[] = {
            {"__gc",       Sqlite_gc},
            {"__tostring", Sqlite_tostring},
            {0,            0}
    };
    //=========================================================================



    //************************************
    static const char LUASQLSTMT[] = "Stmt";

    static sqlite3_stmt *toStmt(lua_State *L, int index) {
        sqlite3_stmt *stmt = (sqlite3_stmt *) lua_touserdata(L, index);
        if (stmt == NULL) {
            lua_pushstring(L, "Type error");
            lua_error(L);
        }
        return stmt;
    }
    //=========================================================================

    static sqlite3_stmt *checkStmt(lua_State *L, int index) {
        luaL_checktype(L, index, LUA_TUSERDATA);
        sqlite3_stmt *stmt;
        stmt = (sqlite3_stmt *) luaL_checkudata(L, index, LUASQLSTMT);
        if (stmt == NULL) {
            lua_pushstring(L, "Type error");
            lua_error(L);
        }
        return stmt;
    }
    //=========================================================================

    static sqlite3_stmt *pushStmt(lua_State *L) {
        sqlite3_stmt *stmt = (sqlite3_stmt *) lua_newuserdata(L, sizeof(sqlite3_stmt));
        luaL_getmetatable(L, LUASQLSTMT);
        lua_setmetatable(L, -2);
        return img;
    }
    //=========================================================================

    //
    // called by garbage collection
    //
    static int Stmt_gc(lua_State *L) {
        sqlite3_stmt *stmt = toStmt(L, 1);
        sqlite3_finalize(stmt);
        return 0;
    }
    //=========================================================================

    static int Stmt_tostring(lua_State *L) {
        sqlite3_stmt *stmt = toSqlite(L, 1);
        std::stringstream ss;
        ss << "SQL: " << sqlite3_expanded_sql(stmt);
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

    //************************************


    //
    // usage:
    //
    //   sqlite.new(path [, "RO" | "RW" | "CRW"])
    static int lua_sqlite(lua_State *L) {
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
            lua_pushstring(L, sqlite3_errmsg(hanlde));
            lua_error(L);
            return 0;
        }

        Sqlite *db = pushSqlite(L);
        db->sqlite_handle = handle;
        db->dbname = new std::string(dppath);

        return 1;
    }

    //
    // usage
    //    db = sqlite.new("/path/to/db/file", "RW")
    //    res = sqlite.query(db, "SELECT * FROM test")
    //
    static int lua_sqlite_query(lua_State *L) {
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
        if (lua_isstring(L, 1)) {
            sql = lua_tostring(L, 2);
        }
        else {
            lua_pushstring(L, "No SQL given!");
            lua_error(L);
            return 0;
        }
        sqlite3_stmt *stmt;
        int status =sqlite3_prepare_v2(db->sqlite_handle, sql, strlen(sql), &stmt, NULL);
    }

    void sqliteGlobals(lua_State *L, shttps::Connection &conn, void *user_data) {
        SipiHttpServer *server = (SipiHttpServer *) user_data;
    }


} // namespace
