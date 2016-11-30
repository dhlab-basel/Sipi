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
#include <SipiCache.h>


#include "SipiImage.h"
#include "SipiLua.h"
#include "SipiHttpServer.h"
#include "SipiCache.h"



namespace Sipi {

    char sipiserver[] = "__sipiserver";

    static const char SIMAGE[] = "SipiImage";

    typedef struct {
        SipiImage *image;
        std::string *filename;
    } SImage;


    /*!
     * Get the size of the cache
     * LUA: cache_size = cache.size()
     */
    static int lua_cache_size(lua_State *L) {
        lua_getglobal(L, sipiserver);
        SipiHttpServer *server = (SipiHttpServer *) lua_touserdata(L, -1);
        lua_remove(L, -1); // remove from stack
        SipiCache *cache = server->cache();

        if (cache == NULL) {
            lua_pushnil(L);
            return 1;
        }

        unsigned long long size = cache->getCachesize();

        lua_pushinteger(L, size);
        return 1;
    }
    //=========================================================================

    /*!
     * Get the maximal size of the cache
     * LUA: cache.max_size = cache.max_size()
     */
    static int lua_cache_max_size(lua_State *L) {
        lua_getglobal(L, sipiserver);
        SipiHttpServer *server = (SipiHttpServer *) lua_touserdata(L, -1);
        lua_remove(L, -1); // remove from stack
        SipiCache *cache = server->cache();

        if (cache == NULL) {
            lua_pushnil(L);
            return 1;
        }

        unsigned long long maxsize = cache->getMaxCachesize();

        lua_pushinteger(L, maxsize);
        return 1;
    }
    //=========================================================================

    /*!
     * Get the size of the cache
     * LUA: cache_size = cache.size()
     */
    static int lua_cache_nfiles(lua_State *L) {
        lua_getglobal(L, sipiserver);
        SipiHttpServer *server = (SipiHttpServer *) lua_touserdata(L, -1);
        lua_remove(L, -1); // remove from stack
        SipiCache *cache = server->cache();

        if (cache == NULL) {
            lua_pushnil(L);
            return 1;
        }

        unsigned size = cache->getNfiles();

        lua_pushinteger(L, size);
        return 1;
    }
    //=========================================================================

    /*!
     * Get the size of the cache
     * LUA: cache_size = cache.size()
     */
    static int lua_cache_max_nfiles(lua_State *L) {
        lua_getglobal(L, sipiserver);
        SipiHttpServer *server = (SipiHttpServer *) lua_touserdata(L, -1);
        lua_remove(L, -1); // remove from stack
        SipiCache *cache = server->cache();

        if (cache == NULL) {
            lua_pushnil(L);
            return 1;
        }

        unsigned size = cache->getMaxNfiles();

        lua_pushinteger(L, size);

        return 1;
    }
    //=========================================================================

    /*!
     * Get path to cache dir
     * LUA: cache_path = cache.path()
     */
    static int lua_cache_path(lua_State *L) {
        lua_getglobal(L, sipiserver);
        SipiHttpServer *server = (SipiHttpServer *) lua_touserdata(L, -1);
        lua_remove(L, -1); // remove from stack
        SipiCache *cache = server->cache();

        if (cache == NULL) {
            lua_pushnil(L);
            return 1;
        }
        std::string cpath = cache->getCacheDir();

        lua_pushstring(L, cpath.c_str());
        return 1;
    }
    //=========================================================================

    static void add_one_cache_file(int index, const std::string &canonical, const SipiCache::CacheRecord &cr, void *userdata) {
        lua_State *L = (lua_State *) userdata;

        lua_pushinteger(L, index);
        lua_createtable(L, 0, 4); // table1

        lua_pushstring(L, "canonical");
        lua_pushstring(L, canonical.c_str());
        lua_rawset(L, -3);

        lua_pushstring(L, "origpath");
        lua_pushstring(L, cr.origpath.c_str());
        lua_rawset(L, -3);

        lua_pushstring(L, "cachepath");
        lua_pushstring(L, cr.cachepath.c_str());
        lua_rawset(L, -3);

        lua_pushstring(L, "size");
        lua_pushinteger(L, cr.fsize);
        lua_rawset(L, -3);

        struct tm *tminfo;
        tminfo = localtime(&cr.access_time);
        char timestr[100];
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", tminfo);
        lua_pushstring(L, "last_access");
        lua_pushstring(L, timestr);
        lua_rawset(L, -3);

        lua_rawset(L, -3);

        return;
    }
    //=========================================================================

    static int lua_cache_filelist(lua_State *L) {
        lua_getglobal(L, sipiserver);
        SipiHttpServer *server = (SipiHttpServer *) lua_touserdata(L, -1);
        lua_remove(L, -1); // remove from stack

        int top = lua_gettop(L);

        std::string sortmethod;
        if (top == 1) {
            sortmethod = std::string(lua_tostring(L, 1));
            lua_pop(L, 1);
        }
        SipiCache *cache = server->cache();

        if (cache == NULL) {
            lua_pushnil(L);
            return 1;
        }

        lua_createtable(L, 0, 0); // table1
        if (sortmethod == "AT_ASC") {
            cache->loop(add_one_cache_file, (void *) L, SipiCache::SortMethod::SORT_ATIME_ASC);
        }
        else if (sortmethod == "AT_DESC") {
            cache->loop(add_one_cache_file, (void *) L, SipiCache::SortMethod::SORT_ATIME_DESC);
        }
        else if (sortmethod == "FS_ASC") {
            cache->loop(add_one_cache_file, (void *) L, SipiCache::SortMethod::SORT_FSIZE_ASC);
        }
        else if (sortmethod == "FS_DESC") {
            cache->loop(add_one_cache_file, (void *) L, SipiCache::SortMethod::SORT_FSIZE_DESC);
        }
        else {
            cache->loop(add_one_cache_file, (void *) L);
        }

        return 1;
    }
    //=========================================================================


    static int lua_delete_cache_file(lua_State *L) {
        lua_getglobal(L, sipiserver);
        SipiHttpServer *server = (SipiHttpServer *) lua_touserdata(L, -1);
        lua_remove(L, -1); // remove from stack
        SipiCache *cache = server->cache();

        if (cache == NULL) {
            lua_pushnil(L);
            return 1;
        }

        int top = lua_gettop(L);

        std::string canonical;
        if (top == 1) {
            canonical = std::string(lua_tostring(L, 1));
            lua_pop(L, 1);
            if (cache->remove(canonical)) {
                lua_pushboolean(L, true);
            }
            else {
                lua_pushboolean(L, false);
            }
        }
        else {
            lua_pushboolean(L, false);
        }

        return 1;
    }
    //=========================================================================

    static int lua_purge_cache(lua_State *L) {
        lua_getglobal(L, sipiserver);
        SipiHttpServer *server = (SipiHttpServer *) lua_touserdata(L, -1);
        lua_remove(L, -1); // remove from stack
        SipiCache *cache = server->cache();

        if (cache == NULL) {
            lua_pushnil(L);
            return 1;
        }

        int n = cache->purge(true);
        lua_pushinteger(L, n);

        return 1;
    }
    //=========================================================================

    static const luaL_Reg cache_methods[] = {
            {"size", lua_cache_size},
            {"max_size", lua_cache_max_size},
            {"nfiles", lua_cache_nfiles},
            {"max_nfiles", lua_cache_max_nfiles},
            {"path", lua_cache_path},
            {"filelist", lua_cache_filelist},
            {"delete", lua_delete_cache_file},
            {"purge", lua_purge_cache},
            {0,     0}
    };
    //=========================================================================


    static SImage *toSImage(lua_State *L, int index) {
        SImage *img = (SImage *) lua_touserdata(L, index);
        if (img == NULL) {
            lua_pushstring(L, "Type error");
            lua_error(L);
        }
        return img;
    }
    //=========================================================================

    static SImage *checkSImage(lua_State *L, int index) {
        SImage *img;
        luaL_checktype(L, index, LUA_TUSERDATA);
        img = (SImage *) luaL_checkudata(L, index, SIMAGE);
        if (img == NULL) {
            lua_pushstring(L, "Type error");
            lua_error(L);
        }
        return img;
    }
    //=========================================================================

    static SImage *pushSImage(lua_State *L) {
        SImage *img = (SImage *) lua_newuserdata(L, sizeof(SImage));
        luaL_getmetatable(L, SIMAGE);
        lua_setmetatable(L, -2);
        return img;
    }
    //=========================================================================

    /*!
     * Lua usage:
     *    img = sipi.image.new("filename")
     *    img = sipi.image.new("filename",{region=<iiif-region-string>, size=<iiif-size-string> | reduce=<integer>})
     */
    static int SImage_new(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 1) {
            // throw an error!
            return 0;
        }
        if (!lua_isstring(L, 1)) {
            // thow an error!
            return 0;
        }
        const char *imgpath = lua_tostring(L, 1);

        SipiRegion *region = NULL;
        SipiSize *size = NULL;
        if (top == 2) {
            if (lua_istable(L, 2)) {
                //lua_pop(L,1); // remove filename from stack
            }
            else {
                // throw an error
                return 0;
            }
            lua_pushnil(L);
            while (lua_next(L, 2) != 0) {
                if (lua_isstring(L, -2)) {
                    const char *param = lua_tostring(L, -2);
                   if (strcmp(param, "region") == 0) {
                        if (lua_isstring(L, -1)) {
                            region = new SipiRegion(lua_tostring(L, -1));
                        }
                    }
                    else if (strcmp(param, "size") == 0) {
                        if (lua_isstring(L, -1)) {
                            size = new SipiSize(lua_tostring(L, -1));
                        }
                    }
                    else if (strcmp(param, "reduce") == 0) {
                        if (lua_isnumber(L, -1)) {
                            std::cerr << "REDUCE=" << (int) lua_tointeger(L, -1) << std::endl;
                            size = new SipiSize((int) lua_tointeger(L, -1));
                        }
                        else {
                        }
                    }
                    else {
                    }
                }
                /* removes 'value'; keeps 'key' for next iteration */
                lua_pop(L, 1);
            }

        }

        SImage *img = pushSImage(L);
        img->image = new SipiImage();
        img->filename = new std::string(imgpath);
        img->image->read(imgpath, region, size);

        delete region;
        delete size;

        return 1;
    }
    //=========================================================================

    static int SImage_dims(lua_State *L) {
        int nx, ny;
        int top = lua_gettop(L);
        if (top != 1) {
            // throw an error!
            lua_pushstring(L, "Incorrect number of arguments!");
            lua_error(L);
            return 0;
        }

        if (lua_isstring(L, 1)) {
            const char *imgpath = lua_tostring(L, 1);
            SipiImage img;
            img.getDim(imgpath, nx, ny);
        }
        else {
            SImage *img = checkSImage(L, 1);
            if (img == NULL) return 0;
            nx = img->image->getNx();
            ny = img->image->getNy();
        }

        lua_createtable(L, 0, 2); // table

        lua_pushstring(L, "nx"); // table - "nx"
        lua_pushinteger(L, nx); // table - "nx" - <nx>
        lua_rawset(L, -3); // table

        lua_pushstring(L, "ny"); // table - "ny"
        lua_pushinteger(L, ny); // table - "ny" - <ny>
        lua_rawset(L, -3); // table

        return 1;
    }
    //=========================================================================

    /*!
     * Lua usage:
     *    img::mimetype_consistency("image/jpeg", "myfile.jpg")
     */
    static int SImage_mimetype_consistency(lua_State *L) {
        int top = lua_gettop(L);

        // three arguments are expected
        if (top != 3) {
            // throw an error!
            lua_pushstring(L, "Incorrect number of arguments!");
            lua_error(L);
            return 0;
        }

        // get pointer to SImage
        SImage *img = checkSImage(L, 1);

        // get name of uploaded file
        std::string *path = img->filename;

        // get the indicated mimetype and the original filename
        const char* given_mimetype = lua_tostring(L, 2);
        const char* given_filename = lua_tostring(L, 3);

        // do the consistency check
        bool check = img->image->checkMimeTypeConsistency(*path, given_mimetype, given_filename);

        lua_pushboolean(L, check);

        return 1;

    }
    //=========================================================================

    /*!
     * SipiImage.crop(img, <iiif-region>)
     */
    static int SImage_crop(lua_State *L) {
        SImage *img = checkSImage(L, 1);

        if (!lua_isstring(L, 2)) {
            // thow an error!
            return 0;
        }
        const char *regionstr = lua_tostring(L, 2);
        lua_pop(L, 2);

        SipiRegion *reg = new SipiRegion(regionstr);
        img->image->crop(reg);
        delete reg;

        return 0;
    }
    //=========================================================================

    /*!
    * SipiImage.scale(img, <wm-file>)
    */
    static int SImage_scale(lua_State *L) {
        SImage *img = checkSImage(L, 1);

        if (!lua_isstring(L, 2)) {
            // throw an error!
            return 0;
        }
        const char *sizestr = lua_tostring(L, 2);
        lua_pop(L, 2);

        SipiSize *size = new SipiSize(sizestr);
        img->image->scale(512,512);
        delete size;

        return 0;
    }
    //=========================================================================

    /*!
    * SipiImage.scale(img, <wm-file>)
    */
    static int SImage_rotate(lua_State *L) {
        SImage *img = checkSImage(L, 1);

        if (!lua_isnumber(L, 2)) {
            // throw an error!
            return 0;
        }
        float angle = lua_tonumber(L, 2);
        lua_pop(L, 2);

        img->image->rotate(angle);

        return 0;
    }
    //=========================================================================

    /*!
     * SipiImage.watermark(img, <wm-file>)
     */
    static int SImage_watermark(lua_State *L) {
        SImage *img = checkSImage(L, 1);

        if (!lua_isstring(L, 2)) {
            // thow an error!
            return 0;
        }
        const char *watermark = lua_tostring(L, 2);
        lua_pop(L, 2);

        img->image->add_watermark(watermark);

        return 0;
    }
    //=========================================================================


    /*!
     * SipiImage.write(img, <filepath>)
     */
    static int SImage_write(lua_State *L) {
        SImage *img = checkSImage(L, 1);

        if (!lua_isstring(L, 2)) {
            // thow an error!
            return 0;
        }
        const char *imgpath = lua_tostring(L, 2);
        lua_pop(L, 2);

        std::string filename = imgpath;
        size_t pos = filename.find_last_of(".");
        std::string basename = filename.substr(0, pos);
        std::string extension = filename.substr(pos+1);
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        std::string ftype;
        if ((extension == "tif") || (extension == "tiff")) {
            ftype = "tif";
        }
        else if ((extension == "jpg") || (extension == "jpeg")) {
            ftype = "jpg";
        }
        else if (extension == "png") {
            ftype = "png";
        }
        else if ((extension == "j2k") || (extension == "jpx") || (extension == "jp2")) {
            ftype = "jpx";
        }
        else {
        }

        if ((basename == "http") || (basename == "HTTP")) {
            lua_getglobal(L, shttps::luaconnection); // push onto stack
            shttps::Connection *conn = (shttps::Connection *) lua_touserdata(L, -1); // does not change the stack
            lua_remove(L, -1); // remove from stack
            img->image->connection(conn);
            try {
                img->image->write(ftype, "HTTP");
            }
            catch (SipiImageError &err) {
                lua_pushstring(L, err.what());
                return lua_error(L);
            }
        }
        else {
            try {
                img->image->write(ftype, imgpath);
            }
            catch (SipiImageError &err) {
                lua_pushstring(L, err.what());
                return lua_error(L);
            }
        }

        return 0;
    }
    //=========================================================================


    /*!
     * SipiImage.send(img, <format>)
     */
    static int SImage_send(lua_State *L) {
       SImage *img = checkSImage(L, 1);

       if (!lua_isstring(L, 2)) {
           // throw an error!
           return 0;
       }
       const char *ext = lua_tostring(L, 2);
       lua_pop(L, 2);

       std::string extension = ext;
	   std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
       std::string ftype;
       if ((extension == "tif") || (extension == "tiff")) {
           ftype = "tif";
       }
       else if ((extension == "jpg") || (extension == "jpeg")) {
           ftype = "jpg";
       }
       else if (extension == "png") {
           ftype = "png";
       }
       else if ((extension == "j2k") || (extension == "jpx")) {
           ftype = "jpx";
       }
       else {
       }

       lua_getglobal(L, shttps::luaconnection); // push onto stack
       shttps::Connection *conn = (shttps::Connection *) lua_touserdata(L, -1); // does not change the stack
       lua_remove(L, -1); // remove from stack

	   img->image->connection(conn);
       img->image->write(ftype, "HTTP");

       return 0;
   }
   //=========================================================================

    // map the method names exposed to Lua to the names defined here
    static const luaL_Reg SImage_methods[] = {
            {"new", SImage_new},
            {"dims", SImage_dims},
            {"write", SImage_write},
            {"send", SImage_send},
            {"crop", SImage_crop},
            {"scale", SImage_scale},
            {"rotate", SImage_rotate},
            {"watermark", SImage_watermark},
            {"mimetype_consistency", SImage_mimetype_consistency},
            {0,     0}
    };
    //=========================================================================

    static int SImage_gc(lua_State *L) {
        SImage *img = toSImage(L, 1);
        std::cerr << "Deleting SipiImage!" << std::endl;
        delete img->image;
        delete img->filename;
        return 0;
    }
    //=========================================================================

    static int SImage_tostring(lua_State *L) {
        SImage *img = toSImage(L, 1);
        std::stringstream ss;
        ss << "File: " << *(img->filename);
        ss << *(img->image);
        lua_pushstring(L, ss.str().c_str());
        return 1;
    }
    //=========================================================================

    static const luaL_Reg SImage_meta[] = {
            {"__gc",       SImage_gc},
            {"__tostring", SImage_tostring},
            {0,            0}
    };
    //=========================================================================


    void sipiGlobals(lua_State *L, shttps::Connection &conn, void *user_data) {
        SipiHttpServer *server = (SipiHttpServer *) user_data;

        //
        // filesystem functions
        //
        //lua_pushstring(L, "cache"); // table1 - "fs"
        lua_newtable(L); // table
        luaL_setfuncs(L, cache_methods, 0);
        //lua_rawset(L, -3); // table
        lua_setglobal(L, "cache");

        lua_getglobal(L, SIMAGE);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
        }
        // stack:  table(SIMAGE)

        luaL_setfuncs(L, SImage_methods, 0);
        // stack:  table(SIMAGE)

        luaL_newmetatable(L, SIMAGE); // create metatable, and add it to the Lua registry
        // stack: table(SIMAGE) - metatable

        luaL_setfuncs(L, SImage_meta, 0);
        lua_pushliteral(L, "__index");
        // stack: table(SIMAGE) - metatable - "__index"

        lua_pushvalue(L, -3); // dup methods table
        // stack: table(SIMAGE) - metatable - "__index" - table(SIMAGE)

        lua_rawset(L, -3); // metatable.__index = methods
        // stack: table(SIMAGE) - metatable

        lua_pushliteral(L, "__metatable");
        // stack: table(SIMAGE) - metatable - "__metatable"

        lua_pushvalue(L, -3); // dup methods table
        // stack: table(SIMAGE) - metatable - "__metatable" - table(SIMAGE)

        lua_rawset(L, -3);
        // stack: table(SIMAGE) - metatable

        lua_pop(L, 1); // drop metatable
        // stack: table(SIMAGE)

        lua_setglobal(L, SIMAGE);
        // stack: empty

        lua_pushlightuserdata(L, server);
        lua_setglobal(L, sipiserver);

    }
    //=========================================================================

} // namespace Sipi
