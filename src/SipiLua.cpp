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
        }
        lua_pop(L, top);
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

        int top = lua_gettop(L);

        if (cache == NULL) {
            lua_pop(L, top);
            lua_pushnil(L);
            return 1;
        }

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
            lua_pop(L, top);
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
            lua_pushstring(L, "Type error! Not userdata object");
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
            lua_pushstring(L, "Type error! Expected an SipiImage!");
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
     *    img = SipiImage.new("filename")
     *    img = SipiImage.new("filename",{region=<iiif-region-string>, size=<iiif-size-string> , reduce=<integer>, original=origfilename}, hash="md5"|"sha1"|"sha256"|"sha384"|"sha512")
     */
    static int SImage_new(lua_State *L) {
        int top = lua_gettop(L);
        if (top < 1) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "'SipiImage.new()': No filename given!");
            return 2;
        }
        if (!lua_isstring(L, 1)) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "'SipiImage.new()': filename must be string!");
            return 2;
        }
        const char *imgpath = lua_tostring(L, 1);

        SipiRegion *region = NULL;
        SipiSize *size = NULL;
        std::string original;
        shttps::HashType htype = shttps::HashType::sha256;
        if (top == 2) {
            if (lua_istable(L, 2)) {
                //lua_pop(L,1); // remove filename from stack
            }
            else {
                lua_pop(L, top);
                lua_pushboolean(L, false);
                lua_pushstring(L, "'SipiImage.new()': Second parameter must be table!");
                return 2;
            }
            lua_pushnil(L);
            while (lua_next(L, 2) != 0) {
                if (lua_isstring(L, -2)) {
                    const char *param = lua_tostring(L, -2);
                   if (strcmp(param, "region") == 0) {
                        if (lua_isstring(L, -1)) {
                            region = new SipiRegion(lua_tostring(L, -1));
                        }
                        else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "'SipiImage.new()': Error in region parameter!");
                            return 2;
                        }
                    }
                    else if (strcmp(param, "size") == 0) {
                        if (lua_isstring(L, -1)) {
                            size = new SipiSize(lua_tostring(L, -1));
                        }
                        else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "'SipiImage.new()': Error in size parameter!");
                            return 2;
                        }
                    }
                    else if (strcmp(param, "reduce") == 0) {
                        if (lua_isnumber(L, -1)) {
                            size = new SipiSize((int) lua_tointeger(L, -1));
                        }
                        else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "'SipiImage.new()': Error in reduce parameter!");
                            return 2;
                        }
                    }
                    else if (strcmp(param, "original") == 0) {
                        if (lua_isstring(L, -1)) {
                            const char *tmpstr = lua_tostring(L, -1);
                            original = tmpstr;
                        }
                        else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "'SipiImage.new()': Error in original parameter!");
                            return 2;
                        }
                    }
                    else if (strcmp(param, "hash") == 0) {
                        if (lua_isstring(L, -1)) {
                            const char *tmpstr = lua_tostring(L, -1);
                            std::string hashstr = tmpstr;
                            if (hashstr == "md5") {
                                htype = shttps::HashType::md5;
                            }
                            else if (hashstr == "sha1") {
                                htype = shttps::HashType::sha1;
                            }
                            else if (hashstr == "sha256") {
                                htype = shttps::HashType::sha256;
                            }
                            else if (hashstr == "sha384") {
                                htype = shttps::HashType::sha384;
                            }
                            else if (hashstr == "sha512") {
                                htype = shttps::HashType::sha512;
                            }
                            else {
                                lua_pop(L, lua_gettop(L));
                                lua_pushboolean(L, false);
                                lua_pushstring(L, "'SipiImage.new()': Error in hash type!");
                                return 2;
                            }
                        }
                        else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "'SipiImage.new()': Error in hash parameter!");
                            return 2;
                        }
                    }
                    else {
                        lua_pop(L, lua_gettop(L));
                        lua_pushboolean(L, false);
                        lua_pushstring(L, "'SipiImage.new()': Error in parameter table (unknown parameter)!");
                        return 2;
                    }
                }
                /* removes 'value'; keeps 'key' for next iteration */
                lua_pop(L, 1);
            }
        }

        lua_pushboolean(L, true); // result code
        SImage *img = pushSImage(L);
        img->image = new SipiImage();
        img->filename = new std::string(imgpath);
        try {
            if (!original.empty()) {
                img->image->readOriginal(imgpath, region, size, original, htype);
            }
            else {
                img->image->read(imgpath, region, size);
            }
        }
        catch(SipiImageError &err) {
            delete region;
            delete size;
            lua_pop(L, lua_gettop(L));
            lua_pushboolean(L, false);
            std::stringstream ss;
            ss << "'SipiImage.new()': ";
            ss << err;
            lua_pushstring(L, ss.str().c_str());
            return 2;
        }

        delete region;
        delete size;

        return 2;
    }
    //=========================================================================

    static int SImage_dims(lua_State *L) {
        size_t nx, ny;
        int top = lua_gettop(L);
        if (top != 1) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "'SipiImage.dims()': Incorrect number of arguments!");
            return 2;
        }

        if (lua_isstring(L, 1)) {
            const char *imgpath = lua_tostring(L, 1);
            SipiImage img;
            try {
                img.getDim(imgpath, nx, ny);
            }
            catch (SipiImageError &err) {
                lua_pop(L, top);
                lua_pushboolean(L, false);
                std::stringstream ss;
                ss << "'SipiImage.dims()': " << err;
                lua_pushstring(L, ss.str().c_str());
                return 2;
            }
        }
        else {
            SImage *img = checkSImage(L, 1);
            if (img == NULL) {
                lua_pop(L, top);
                lua_pushboolean(L, false);
                lua_pushstring(L, "'SipiImage.dims()': not a valid image!");
                return 2;
            }
            nx = img->image->getNx();
            ny = img->image->getNy();
        }

        lua_pushboolean(L, true);
        lua_createtable(L, 0, 2); // table

        lua_pushstring(L, "nx"); // table - "nx"
        lua_pushinteger(L, nx); // table - "nx" - <nx>
        lua_rawset(L, -3); // table

        lua_pushstring(L, "ny"); // table - "ny"
        lua_pushinteger(L, ny); // table - "ny" - <ny>
        lua_rawset(L, -3); // table

        return 2;
    }
    //=========================================================================

    /*!
     * Lua usage:
     *    SipiImage.mimetype_consistency(img, "image/jpeg", "myfile.jpg")
     */
    static int SImage_mimetype_consistency(lua_State *L) {
        int top = lua_gettop(L);

        // three arguments are expected
        if (top != 3) {
            lua_pop(L, top); // clear stack
            lua_pushboolean(L, false);
            lua_pushstring(L, "'SipiImage.mimetype_consistency()': Incorrect number of arguments!");
            return 2;
        }

        // get pointer to SImage
        SImage *img = checkSImage(L, 1);

        // get name of uploaded file
        std::string *path = img->filename;

        // get the indicated mimetype and the original filename
        const char* given_mimetype = lua_tostring(L, 2);
        const char* given_filename = lua_tostring(L, 3);

        lua_pop(L, top); // clear stack

        // do the consistency check
        bool check;
        try {
            check = img->image->checkMimeTypeConsistency(*path, given_mimetype, given_filename);
        }
        catch (SipiImageError &err) {
            lua_pushboolean(L, false);
            std::stringstream ss;
            ss << "'SipiImage.mimetype_consistency()': " << err;
            lua_pushstring(L, ss.str().c_str());
            return 2;
        }

        lua_pushboolean(L, true); // status
        lua_pushboolean(L, check); // result

        return 2;

    }
    //=========================================================================

    /*!
     * SipiImage.crop(img, <iiif-region>)
     */
    static int SImage_crop(lua_State *L) {
        SImage *img = checkSImage(L, 1);

        int top = lua_gettop(L);
        if (!lua_isstring(L, 2)) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "'SipiImage.crop()': Incorrect number of arguments!");
            return 2;
        }
        const char *regionstr = lua_tostring(L, 2);
        lua_pop(L, top);

        SipiRegion *reg;
        try {
            reg = new SipiRegion(regionstr);
        }
        catch (SipiError &err) {
            lua_pushboolean(L, false);
            std::stringstream ss;
            ss << "'SipiImage.crop()': " << err;
            lua_pushstring(L, ss.str().c_str());
            return 2;
        }
        img->image->crop(reg); // can not throw exception!
        delete reg;

        lua_pushboolean(L, true);
        lua_pushnil(L);

        return 2;
    }
    //=========================================================================

    /*!
    * SipiImage.scale(img, sizestring)
    */
    static int SImage_scale(lua_State *L) {
        SImage *img = checkSImage(L, 1);

        int top = lua_gettop(L);
        if (!lua_isstring(L, 2)) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "'SipiImage.scale()': Incorrect number of arguments!");
            return 2;
        }
        const char *sizestr = lua_tostring(L, 2);
        lua_pop(L, top);

        SipiSize *size;
        size_t nx, ny;
        try {
            size = new SipiSize(sizestr);
            int r;
            bool ro;
            size->get_size(img->image->getNx(), img->image->getNy(), nx, ny, r, ro);
        }
        catch (SipiError &err) {
            lua_pushboolean(L, false);
            std::stringstream ss;
            ss << "'SipiImage.scale()': " << err;
            lua_pushstring(L, ss.str().c_str());
            return 2;
        }

        img->image->scale(nx, ny);
        delete size;

        lua_pushboolean(L, true);
        lua_pushnil(L);

        return 2;
    }
    //=========================================================================

    /*!
    * SipiImage.rotate(img, number)
    */
    static int SImage_rotate(lua_State *L) {
        int top = lua_gettop(L);
        if (top != 2) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "'SipiImage.rotate()': Incorrect number of arguments!");
            return 2;
        }
        SImage *img = checkSImage(L, 1);

        if (!lua_isnumber(L, 2)) {
            lua_pushboolean(L, false);
            lua_pushstring(L, "'SipiImage.rotate()': Incorrect  arguments!");
            return 2;
        }
        float angle = lua_tonumber(L, 2);
        lua_pop(L, top);

        img->image->rotate(angle); // does not throw an exception!

        lua_pushboolean(L, true);
        lua_pushnil(L);

        return 2;
    }
    //=========================================================================

    /*!
     * SipiImage.watermark(img, <wm-file>)
     */
    static int SImage_watermark(lua_State *L) {
        int top = lua_gettop(L);

        SImage *img = checkSImage(L, 1);

        if (!lua_isstring(L, 2)) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "'SipiImage.watermark()': Incorrect arguments!");
            return 2;
        }
        const char *watermark = lua_tostring(L, 2);
        lua_pop(L, top);

        try {
            img->image->add_watermark(watermark);
        }
        catch(SipiImageError &err) {
            lua_pushboolean(L, false);
            std::stringstream ss;
            ss << "'SipiImage.watermark()': " << err;
            lua_pushstring(L, ss.str().c_str());
            return 2;
        }

        lua_pushboolean(L, true);
        lua_pushnil(L);

        return 2;
    }
    //=========================================================================


    /*!
     * SipiImage.write(img, <filepath>)
     */
    static int SImage_write(lua_State *L) {
        int top = lua_gettop(L);

        SImage *img = checkSImage(L, 1);

        if (!lua_isstring(L, 2)) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "'SipiImage.write()': Incorrect arguments!");
            return 2;
        }
        const char *imgpath = lua_tostring(L, 2);
        lua_pop(L, 2);

        std::string filename = imgpath;
        size_t pos_ext = filename.find_last_of(".");
        size_t pos_start = filename.find_last_of("/");
        std::string dirpath;
        std::string basename;
        std::string extension;
        if (pos_start == std::string::npos) {
            pos_start = 0;
        }
        else {
            dirpath = filename.substr(0, pos_start);
        }
        if (pos_ext != std::string::npos) {
            basename = filename.substr(pos_start, pos_ext - pos_start);
            extension = filename.substr(pos_ext + 1);
        }
        else {
            basename = filename.substr(pos_start);
        }
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
            lua_pushstring(L, "'SipiImage.write()': unsupported file format!");
            return lua_error(L);
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
                lua_pop(L, top);
                lua_pushboolean(L, false);
                lua_pushstring(L, err.to_string().c_str());
                return 2;
            }
        }
        else {
            try {
                img->image->write(ftype, imgpath);
            }
            catch (SipiImageError &err) {
                lua_pop(L, top);
                lua_pushboolean(L, false);
                lua_pushstring(L, err.to_string().c_str());
                return 2;
            }
        }

        lua_pushboolean(L, true);
        lua_pushnil(L);

        return 2;
    }
    //=========================================================================


    /*!
    * SipiImage.send(img, <format>)
    */
    static int SImage_send(lua_State *L) {
        int top = lua_gettop(L);
        SImage *img = checkSImage(L, 1);

        if (!lua_isstring(L, 2)) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "'SipiImage.send()': Incorrect arguments!");
            return 2;
        }
        const char *ext = lua_tostring(L, 2);
        lua_pop(L, top);

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
            lua_pushstring(L, "'SipiImage.send()': unsupported file format!");
            return lua_error(L);
        }

        lua_getglobal(L, shttps::luaconnection); // push onto stack
        shttps::Connection *conn = (shttps::Connection *) lua_touserdata(L, -1); // does not change the stack
        lua_remove(L, -1); // remove from stack

        img->image->connection(conn);
        try {
            img->image->write(ftype, "HTTP");
        }
        catch (SipiImageError &err) {
            lua_pushboolean(L, false);
            lua_pushstring(L, err.to_string().c_str());
            return 2;
        }

        lua_pushboolean(L, true);
        lua_pushnil(L);

        return 2;
    }
   //=========================================================================

    // map the method names exposed to Lua to the names defined here
    static const luaL_Reg SImage_methods[] = {
            {"new", SImage_new},
            {"dims", SImage_dims}, // #myimg
            {"write", SImage_write}, // myimg >> filename
            {"send", SImage_send}, // myimg
            {"crop", SImage_crop}, // myimg - "100,100,500,500"
            {"scale", SImage_scale}, // myimg % "500,"
            {"rotate", SImage_rotate}, // myimg * 45.0
            {"watermark", SImage_watermark}, // myimg + "wm-path"
            {"mimetype_consistency", SImage_mimetype_consistency},
            {0,     0}
    };
    //=========================================================================

    static int SImage_gc(lua_State *L) {
        SImage *img = toSImage(L, 1);
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
