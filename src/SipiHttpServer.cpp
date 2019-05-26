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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <syslog.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>
#include <utility>
#include <regex>
#include <sys/stat.h>

#include <SipiFilenameHash.h>

#include "SipiImage.h"
#include "SipiError.h"
#include "iiifparser/SipiQualityFormat.h"
#include "iiifparser/SipiIdentifier.h"
#include "PhpSession.h"
// #include "Salsah.h"

#include "shttps/Global.h"
#include "SipiHttpServer.h"
#include "shttps/Connection.h"
#include "shttps/Parsing.h"

#include "jansson.h"
#include "favicon.h"

#include "lua.hpp"

using namespace shttps;
static const char __file__[] = __FILE__;

namespace Sipi {
    /*!
     * The name of the Lua function that checks permissions before a file is returned to an HTTP client.
     */
    static const std::string pre_flight_func_name = "pre_flight";

    typedef enum {
        iiif_prefix = 0,            //!< http://{url}/*{prefix}*/{id}/{region}/{size}/{rotation}/{quality}.{format}
        iiif_identifier = 1,        //!< http://{url}/{prefix}/*{id}*/{region}/{size}/{rotation}/{quality}.{format}
        iiif_region = 2,            //!< http://{url}/{prefix}/{id}/{region}/{size}/{rotation}/{quality}.{format}
        iiif_size = 3,              //!< http://{url}/{prefix}/{id}/{region}/*{size}*/{rotation}/{quality}.{format}
        iiif_rotation = 4,          //!< http://{url}/{prefix}/{id}/{region}/{size}/*{rotation}*/{quality}.{format}
        iiif_qualityformat = 5,     //!< http://{url}/{prefix}/{id}/{region}/{size}/{rotation}/*{quality}.{format}*
    } IiifParams;

    /*!
     * Sends an HTTP error response to the client, and logs the error if appropriate.
     *
     * \param conn_obj the server connection.
     * \param code the HTTP status code to be returned.
     * \param errmsg the error message to be returned.
     */
    static void send_error(Connection &conn_obj, Connection::StatusCodes code, const std::string &errmsg) {
        conn_obj.status(code);
        conn_obj.setBuffer();
        conn_obj.header("Content-Type", "text/plain");

        std::string http_err_name;
        bool log_err(true); // True if the error should be logged.

        switch (code) {
            case Connection::BAD_REQUEST:
                http_err_name = "Bad Request";
                // log_err = false;
                break;

            case Connection::FORBIDDEN:
                http_err_name = "Forbidden";
                // log_err = false;
                break;

            case Connection::UNAUTHORIZED:
                http_err_name = "Unauthorized";
                break;

            case Connection::NOT_FOUND:
                http_err_name = "Not Found";
                // log_err = false;
                break;

            case Connection::INTERNAL_SERVER_ERROR:
                http_err_name = "Internal Server Error";
                break;

            case Connection::NOT_IMPLEMENTED:
                http_err_name = "Not Implemented";
                // log_err = false;
                break;

            case Connection::SERVICE_UNAVAILABLE:
                http_err_name = "Service Unavailable";
                break;

            default:
                http_err_name = "Unknown error";
                break;
        }

        // Send an error message to the client.

        conn_obj << http_err_name;

        if (!errmsg.empty()) {
            conn_obj << ": " << errmsg;
        }

        conn_obj.flush();

        // Log the error if appropriate.

        if (log_err) {
            std::stringstream log_msg_stream;
            log_msg_stream << "GET " << conn_obj.uri() << " failed (" << http_err_name << ")";


            if (!errmsg.empty()) {
                log_msg_stream << ": " << errmsg;
            }

            syslog(LOG_ERR, "%s", log_msg_stream.str().c_str());
        }

    }
    //=========================================================================

    /*!
     * Sends an HTTP error response to the client, and logs the error if appropriate.
     *
     * \param conn_obj the server connection.
     * \param code the HTTP status code to be returned.
     * \param err an exception describing the error.
     */
    static void send_error(Connection &conn_obj, Connection::StatusCodes code, const SipiError &err) {
        send_error(conn_obj, code, err.to_string());
    }
    //=========================================================================

    /*!
     * Sends an HTTP error response to the client, and logs the error if appropriate.
     *
     * \param conn_obj the server connection.
     * \param code the HTTP status code to be returned.
     */
    static void send_error(Connection &conn_obj, Connection::StatusCodes code) {
        send_error(conn_obj, code, "");
    }
    //=========================================================================

    /*!
     * Gets the IIIF prefix, IIIF identifier, and cookie from the HTTP request, and passes them to the Lua pre-flight function (whose
     * name is given by the constant pre_flight_func_name).
     *
     * Returns the return values of the pre-flight function as a std::pair containing a permission string and (optionally) a file path.
     * Throws SipiError if an error occurs.
     *
     * \param conn_obj the server connection.
     * \param luaserver the Lua server that will be used to call the function.
     * \param params the HTTP request parameters.
     * \return Pair of permission string and filepath
     */
    static std::pair<std::string, std::string>
    call_pre_flight(Connection &conn_obj, shttps::LuaServer &luaserver, const std::string &prefix, const std::string &identifier) {
        // The permission and optional file path that the pre_fight function returns.
        std::string permission;
        std::string infile;

        // The paramters to be passed to the pre-flight function.
        std::vector<LuaValstruct> lvals;

        // The first parameter is the IIIF prefix.
        LuaValstruct iiif_prefix_param;
        iiif_prefix_param.type = LuaValstruct::STRING_TYPE;
        iiif_prefix_param.value.s = prefix;
        lvals.push_back(iiif_prefix_param);

        // The second parameter is the IIIF identifier.
        LuaValstruct iiif_identifier_param;
        iiif_identifier_param.type = LuaValstruct::STRING_TYPE;
        iiif_identifier_param.value.s = identifier;
        lvals.push_back(iiif_identifier_param);

        // The third parameter is the HTTP cookie.
        LuaValstruct cookie_param;
        std::string cookie = conn_obj.header("cookie");
        cookie_param.type = LuaValstruct::STRING_TYPE;
        cookie_param.value.s = cookie;
        lvals.push_back(cookie_param);

        // Call the pre-flight function.
        std::vector<LuaValstruct> rvals = luaserver.executeLuafunction(pre_flight_func_name, lvals);

        // If it returned nothing, that's an error.
        if (rvals.empty()) {
            std::ostringstream err_msg;
            err_msg << "Lua function " << pre_flight_func_name << " must return at least one value";
            throw SipiError(__file__, __LINE__, err_msg.str());
        }

        // The first return value is the permission code.
        auto permission_return_val = rvals.at(0);

        // The permission code must be a string.
        if (permission_return_val.type == LuaValstruct::STRING_TYPE) {
            permission = permission_return_val.value.s;
        } else {
            std::ostringstream err_msg;
            err_msg << "The permission value returned by Lua function " << pre_flight_func_name << " was not a string";
            throw SipiError(__file__, __LINE__, err_msg.str());
        }

        // If the permission code is "allow" or begins with "restrict", there must also be a file path.
        if (permission == "allow" || permission.find("restrict") == 0) {
            if (rvals.size() == 2) {
                auto infile_return_val = rvals.at(1);

                // The file path must be a string.
                if (infile_return_val.type == LuaValstruct::STRING_TYPE) {
                    infile = infile_return_val.value.s;
                } else {
                    std::ostringstream err_msg;
                    err_msg << "The file path returned by Lua function " << pre_flight_func_name << " was not a string";
                    throw SipiError(__file__, __LINE__, err_msg.str());
                }
            } else {
                std::ostringstream err_msg;
                err_msg << "Lua function " << pre_flight_func_name
                        << " returned permission 'allow', but it did not return a file path";
                throw SipiError(__file__, __LINE__, err_msg.str());
            }
        }

        // Return the permission code and file path, if any, as a std::pair.
        return std::make_pair(permission, infile);
    }
    //=========================================================================

    enum AccessError {PRE_FLIGHT_DENY,  FILE_ACCESS_DENY};

    static std::pair<std::string,int> check_file_access(
            Connection &conn_obj,
            SipiHttpServer *serv,
            shttps::LuaServer &luaserver,
            std::vector<std::string> &params,
            bool prefix_as_path) {
        std::string infile;

        SipiIdentifier sid = SipiIdentifier(params[iiif_identifier]);
        int pagenum = sid.getPage();
        if (luaserver.luaFunctionExists(pre_flight_func_name)) {
            std::pair<std::string, std::string> pre_flight_return_values;

            pre_flight_return_values = call_pre_flight(conn_obj, luaserver, urldecode(params[iiif_prefix]), sid.getIdentifier()); // may throw SipiError

            std::string permission = pre_flight_return_values.first;
            infile = pre_flight_return_values.second;

            //
            // here we adjust the path for the subdirs
            //
            if (SipiFilenameHash::getLevels() > 0) {
                bool use_subdirs = true;
                if (prefix_as_path) {
                    for (auto str: serv->dirs_to_exclude()) {
                        if (str == urldecode(params[iiif_prefix])) {
                            use_subdirs = false; // prefix is in list which is excluded from usiong subdirs
                        }
                    }
                }
                if (use_subdirs) {
                    size_t ppos = infile.rfind('/');
                    if ((ppos != std::string::npos) && (ppos < (infile.size() - 1))) {
                        std::string dirpart = infile.substr(0, ppos);
                        std::string filepart = infile.substr(ppos + 1);
                        SipiFilenameHash identifier = SipiFilenameHash(filepart);
                        infile = dirpart + "/" + identifier.filepath();
                    }
                }
            }

            if (permission != "allow") {
                throw PRE_FLIGHT_DENY;
            }
        } else {
            SipiFilenameHash identifier = SipiFilenameHash(sid.getIdentifier());
            if (prefix_as_path) {
                bool use_subdirs = true;
                for (auto str: serv->dirs_to_exclude()) {
                    if (str == urldecode(params[iiif_prefix])) {
                        use_subdirs = false; // prefix is in list which is excluded from usiong subdirs
                    }
                }
                if (use_subdirs) {
                    infile = serv->imgroot() + "/" + urldecode(params[iiif_prefix]) + "/" +
                             identifier.filepath();
                }
                else {
                    infile = serv->imgroot() + "/" + urldecode(params[iiif_prefix]) + "/" +
                            sid.getIdentifier();
                }
            } else {
                infile = serv->imgroot() + "/" + identifier.filepath();
            }
        }
        //
        // test if we have access to the file
        //
        if (access(infile.c_str(), R_OK) != 0) { // test, if file exists
            throw FILE_ACCESS_DENY;
        }

        std::pair<std::string,int> result(infile, pagenum);
        return result;
    }
    //=========================================================================

    static void iiif_send_info(Connection &conn_obj, SipiHttpServer *serv, shttps::LuaServer &luaserver,
                               std::vector<std::string> &params, bool prefix_as_path) {
        conn_obj.setBuffer(); // we want buffered output, since we send JSON text...
        const std::string contenttype = conn_obj.header("accept");

        conn_obj.header("Access-Control-Allow-Origin", "*");
        //
        // here we start the lua script which checks for permissions
        //

        //std::string permission;
        std::pair<std::string,int> access;
        try {
            access = check_file_access(conn_obj, serv, luaserver, params, prefix_as_path);
        }
        catch(AccessError x) {
            switch (x) {
                case PRE_FLIGHT_DENY: {
                    send_error(conn_obj, Connection::UNAUTHORIZED, "Unauthorized access");
                    return;
                }
                case FILE_ACCESS_DENY: {
                    send_error(conn_obj, Connection::BAD_REQUEST, "File not readable");
                    return;
                }
            }
        }
        catch (SipiError &err) {
            send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err);
            return;
        }
        std::string infile = access.first;
        int pagenum = access.second;

        if (!contenttype.empty() && (contenttype == "application/ld+json")) {
            conn_obj.header("Content-Type", "application/ld+json");
        } else {
            conn_obj.header("Content-Type", "application/json");
            conn_obj.header("Link",
                            "<http://iiif.io/api/image/2/context.json>; rel=\"http://www.w3.org/ns/json-ld#context\"; type=\"application/ld+json\"");
        }

        json_t *root = json_object();
        json_object_set_new(root, "@context", json_string("http://iiif.io/api/image/2/context.json"));

        std::string host = conn_obj.header("host");
        std::string id;
        if (conn_obj.secure()) {
            id = std::string("https://") + host + "/" + params[iiif_prefix] + "/" + params[iiif_identifier];
        }
        else {
            id = std::string("http://") + host + "/" + params[iiif_prefix] + "/" + params[iiif_identifier];
        }
        json_object_set_new(root, "@id", json_string(id.c_str()));

        json_object_set_new(root, "protocol", json_string("http://iiif.io/api/image"));

        size_t width, height;
        int numpages = 0;

        //
        // get cache info
        //
        std::shared_ptr<SipiCache> cache = serv->cache();

        if ((cache == nullptr) || !cache->getSize(infile, width, height, pagenum)) {
            Sipi::SipiImage tmpimg;
            Sipi::SipiImgInfo info;
            try {
                info = tmpimg.getDim(infile, pagenum);
            }
            catch (SipiImageError &err) {
                send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err.to_string());
                return;
            }
            if (info.success == SipiImgInfo::FAILURE) {
                send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, "Error getting image dimensions!");
                return;
            }
            width = info.width;
            height = info.height;
            numpages = info.numpages;
        }

        json_object_set_new(root, "width", json_integer(width));
        json_object_set_new(root, "height", json_integer(height));
        if (numpages > 0) {
            json_object_set_new(root, "numpages", json_integer(numpages));
        }

        json_t *sizes = json_array();

        for (int i = 1; i < 5; i++) {
            SipiSize size(i);
            size_t w, h;
            int r;
            bool ro;
            size.get_size(width, height, w, h, r, ro);
            if ((w < 128) && (h < 128)) break;
            json_t *sobj = json_object();
            json_object_set_new(sobj, "width", json_integer(w));
            json_object_set_new(sobj, "height", json_integer(h));
            json_array_append_new(sizes, sobj);
        }

        json_object_set_new(root, "sizes", sizes);
        json_t *profile_arr = json_array();
        json_array_append_new(profile_arr, json_string("http://iiif.io/api/image/2/level2.json"));
        json_t *profile = json_object();
        const char *formats_str[] = {"tif", "jpg", "png", "jp2"};
        json_t *formats = json_array();

        for (unsigned int i = 0; i < sizeof(formats_str) / sizeof(char *); i++) {
            json_array_append_new(formats, json_string(formats_str[i]));
        }

        json_object_set_new(profile, "formats", formats);
        const char *qualities_str[] = {"color", "gray"};
        json_t *qualities = json_array();

        for (unsigned int i = 0; i < sizeof(qualities_str) / sizeof(char *); i++) {
            json_array_append_new(qualities, json_string(qualities_str[i]));
        }

        json_object_set_new(profile, "qualities", qualities);

        const char *supports_str[] = {"color", "cors", "mirroring", "profileLinkHeader", "regionByPct", "regionByPx",
                                      "rotationArbitrary", "rotationBy90s", "sizeAboveFull", "sizeByWhListed",
                                      "sizeByForcedWh", "sizeByH", "sizeByPct", "sizeByW", "sizeByWh"};

        json_t *supports = json_array();

        for (unsigned int i = 0; i < sizeof(supports_str) / sizeof(char *); i++) {
            json_array_append_new(supports, json_string(supports_str[i]));
        }

        json_object_set_new(profile, "supports", supports);
        json_array_append_new(profile_arr, profile);
        json_object_set_new(root, "profile", profile_arr);
        char *json_str = json_dumps(root, JSON_INDENT(3));
        conn_obj.sendAndFlush(json_str, strlen(json_str));
        free(json_str);

        //TODO and all the other CJSON obj?
        json_decref(root);

        syslog(LOG_INFO, "info.json created from: %s", infile.c_str());
    }
    //=========================================================================

    static void knora_send_info(Connection &conn_obj, SipiHttpServer *serv, shttps::LuaServer &luaserver,
                               std::vector<std::string> &params, bool prefix_as_path) {
        conn_obj.setBuffer(); // we want buffered output, since we send JSON text...

        conn_obj.header("Access-Control-Allow-Origin", "*");
        //
        // here we start the lua script which checks for permissions
        //
        std::pair<std::string,int> access;
        try {
            access = check_file_access(conn_obj, serv, luaserver, params, prefix_as_path);
        }
        catch(AccessError x) {
            switch (x) {
                case PRE_FLIGHT_DENY: {
                    send_error(conn_obj, Connection::UNAUTHORIZED, "Unauthorized access");
                    return;
                }
                case FILE_ACCESS_DENY: {
                    send_error(conn_obj, Connection::BAD_REQUEST, "File not readable");
                    return;
                }
            }
        }
        catch (SipiError &err) {
            send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err);
            return;
        }
        std::string infile = access.first;
        int pagenum = access.second;

        conn_obj.header("Content-Type", "application/json");

        json_t *root = json_object();
        int width, height;
        //
        // get cache info
        //
        std::shared_ptr<SipiCache> cache = serv->cache();

        Sipi::SipiImage tmpimg;
        Sipi::SipiImgInfo info;
        try {
            info = tmpimg.getDim(infile, pagenum);
        }
        catch (SipiImageError &err) {
            send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err.to_string());
            return;
        }
        if (info.success == SipiImgInfo::FAILURE) {
            send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, "Error getting image dimensions!");
            return;
        }
        width = info.width;
        height = info.height;

        json_object_set_new(root, "width", json_integer(width));
        json_object_set_new(root, "height", json_integer(height));
        if (info.numpages > 0) {
            json_object_set_new(root, "numpages", json_integer(info.numpages));
        }
        if (info.success == SipiImgInfo::ALL) {
            json_object_set_new(root, "originalMimeType", json_string(info.mimetype.c_str()));
            json_object_set_new(root, "originalFilename", json_string(info.origname.c_str()));
        }
        char *json_str = json_dumps(root, JSON_INDENT(3));
        conn_obj.sendAndFlush(json_str, strlen(json_str));
        free(json_str);

        // TODO: and all the other CJSON obj?
        json_decref(root);

        syslog(LOG_INFO, "knora.json created from: %s", infile.c_str());
    }
    //=========================================================================


    std::pair<std::string, std::string>
    SipiHttpServer::get_canonical_url(size_t tmp_w, size_t tmp_h, const std::string &host, const std::string &prefix,
                                      const std::string &identifier, std::shared_ptr<SipiRegion> region,
                                      std::shared_ptr<SipiSize> size, SipiRotation &rotation,
                                      SipiQualityFormat &quality_format, int pagenum) {
        static const int canonical_len = 127;

        char canonical_region[canonical_len + 1];
        char canonical_size[canonical_len + 1];

        int tmp_r_x, tmp_r_y, tmp_red;
        size_t tmp_r_w, tmp_r_h;
        bool tmp_ro;

        if (region->getType() != SipiRegion::FULL) {
            region->crop_coords(tmp_w, tmp_h, tmp_r_x, tmp_r_y, tmp_r_w, tmp_r_h);
        }

        region->canonical(canonical_region, canonical_len);

        if (size->getType() != SipiSize::FULL) {
            size->get_size(tmp_w, tmp_h, tmp_r_w, tmp_r_h, tmp_red, tmp_ro);
        }

        size->canonical(canonical_size, canonical_len);
        float angle;
        bool mirror = rotation.get_rotation(angle);
        char canonical_rotation[canonical_len + 1];

        if (mirror || (angle != 0.0)) {
            if ((angle - floorf(angle)) < 1.0e-6) { // it's an integer
                if (mirror) {
                    (void) snprintf(canonical_rotation, canonical_len, "!%ld", lround(angle));
                } else {
                    (void) snprintf(canonical_rotation, canonical_len, "%ld", lround(angle));
                }
            } else {
                if (mirror) {
                    (void) snprintf(canonical_rotation, canonical_len, "!%1.1f", angle);
                } else {
                    (void) snprintf(canonical_rotation, canonical_len, "%1.1f", angle);
                }
            }
            syslog(LOG_DEBUG, "Rotation (canonical): %s", canonical_rotation);
        } else {
            (void) snprintf(canonical_rotation, canonical_len, "0");
        }

        const unsigned canonical_header_len = 511;
        char canonical_header[canonical_header_len + 1];
        char ext[5];

        switch (quality_format.format()) {
            case SipiQualityFormat::JPG: {
                ext[0] = 'j';
                ext[1] = 'p';
                ext[2] = 'g';
                ext[3] = '\0';
                break; // jpg
            }

            case SipiQualityFormat::JP2: {
                ext[0] = 'j';
                ext[1] = 'p';
                ext[2] = '2';
                ext[3] = '\0';
                break; // jp2
            }

            case SipiQualityFormat::TIF: {
                ext[0] = 't';
                ext[1] = 'i';
                ext[2] = 'f';
                ext[3] = '\0';
                break; // tif
            }

            case SipiQualityFormat::PNG: {
                ext[0] = 'p';
                ext[1] = 'n';
                ext[2] = 'g';
                ext[3] = '\0';
                break; // png
            }

            case SipiQualityFormat::PDF: {
                ext[0] = 'p';
                ext[1] = 'd';
                ext[2] = 'f';
                ext[3] = '\0';
                break; // pdf
            }

            default: {
                throw SipiError(__file__, __LINE__,
                                "Unsupported file format requested! Supported are .jpg, .jp2, .tif, .png, .pdf");
            }
        }

        std::string format;

        if (quality_format.quality() != SipiQualityFormat::DEFAULT) {
            switch (quality_format.quality()) {
                case SipiQualityFormat::COLOR: {
                    format = "/color.";
                    break;
                }

                case SipiQualityFormat::GRAY: {
                    format = "/gray.";
                    break;
                }

                case SipiQualityFormat::BITONAL: {
                    format = "/bitonal.";
                    break;
                }

                default: {
                    format = "/default.";
                }
            }
        } else {
            format = "/default.";
        }

        std::string fullid = identifier;
        if (pagenum > 0) fullid += "@" + std::to_string(pagenum);
        (void) snprintf(canonical_header, canonical_header_len,
                        "<http://%s/%s/%s/%s/%s/%s/default.%s>;rel=\"canonical\"", host.c_str(), prefix.c_str(),
                        fullid.c_str(), canonical_region, canonical_size, canonical_rotation, ext);
        std::string canonical = host + "/" + prefix + "/" + fullid + "/" + std::string(canonical_region) + "/" +
                                std::string(canonical_size) + "/" + std::string(canonical_rotation) + format +
                                std::string(ext);

        return make_pair(std::string(canonical_header), canonical);
    }
    //=========================================================================


    static void process_get_request(Connection &conn_obj, shttps::LuaServer &luaserver, void *user_data, void *dummy) {
        SipiHttpServer *serv = (SipiHttpServer *) user_data;

        bool prefix_as_path = serv->prefix_as_path();

        std::string uri = conn_obj.uri();

        std::vector<std::string> params;
        size_t pos = 0;
        size_t old_pos = 0;

        //
        // IIIF URi schema:
        // {scheme}://{server}{/prefix}/{identifier}/{region}/{size}/{rotation}/{quality}.{format}
        //
        // The slashes "/" separate the different parts...
        //
        while ((pos = uri.find('/', pos)) != std::string::npos) {
            pos++;

            if (pos == 1) { // if first char is a token skip it!
                old_pos = pos;
                continue;
            }

            params.push_back(uri.substr(old_pos, pos - old_pos - 1));
            old_pos = pos;
        }

        if (old_pos != uri.length()) {
            params.push_back(uri.substr(old_pos, std::string::npos));
        }

        if (params.size() < 1) {
            send_error(conn_obj, Connection::BAD_REQUEST, "No parameters/path given");
            return;
        }

        params.push_back(uri.substr(old_pos, std::string::npos));

        //
        // if we just get the base URL, we redirect to the image info document and the file is
        // a supported image file format. Otherwise we just serve the file as blob.
        // Note: PDF's are in this context blobs and we serve the whole PDF!
        //
        if (params.size() == 3) {
            std::string infile;

            SipiFilenameHash identifier = SipiFilenameHash(urldecode(params[iiif_identifier]));
            if (prefix_as_path) {
                bool use_subdirs = true;
                for (auto str: serv->dirs_to_exclude()) {
                    if (str == urldecode(params[iiif_prefix])) {
                        use_subdirs = false; // prefix is in list which is excluded from using subdirs
                    }
                }
                if (use_subdirs) {
                    infile = serv->imgroot() + "/" + urldecode(params[iiif_prefix]) + "/" +
                             identifier.filepath();
                }
                else {
                    infile = serv->imgroot() + "/" + urldecode(params[iiif_prefix]) + "/" +
                            urldecode(params[iiif_identifier]);
                }
            } else {
                infile = serv->imgroot() + "/" + identifier.filepath();
            }

            syslog(LOG_DEBUG, "GET %s: file %s", uri.c_str(), infile.c_str());

            if (access(infile.c_str(), R_OK) == 0) {
                std::string actual_mimetype = shttps::Parsing::getFileMimetype(infile).first;
                syslog(LOG_DEBUG, "MIMETYPE= %s: file %s", actual_mimetype.c_str(), infile.c_str());
                if ((actual_mimetype == "image/tiff") ||
                    (actual_mimetype == "image/jpeg") ||
                    (actual_mimetype == "image/png") ||
                    (actual_mimetype == "image/jpx") ||
                    (actual_mimetype == "image/jp2")) { // no PDF here!! They are servced as blob
                    conn_obj.setBuffer();
                    conn_obj.status(Connection::SEE_OTHER);
                    const std::string host = conn_obj.header("host");
                    std::string redirect =
                            std::string("http://") + host + "/" + params[iiif_prefix] + "/" + params[iiif_identifier] +
                            "/info.json";
                    conn_obj.header("Location", redirect);
                    conn_obj.header("Content-Type", "text/plain");
                    conn_obj << "Redirect to " << redirect;
                    syslog(LOG_INFO, "GET: redirect to %s", redirect.c_str());
                    conn_obj.flush();
                    return;
                }
                else {
                    //
                    // first we get the filesize and time using fstat
                    //
                    struct stat fstatbuf;

                    if (stat(infile.c_str(), &fstatbuf) != 0) {
                        throw Error(__file__, __LINE__, "Cannot fstat file!");
                    }
                    size_t fsize = fstatbuf.st_size;
                    struct timespec rawtime = fstatbuf.st_mtimespec;
                    char timebuf[100];
                    std::strftime(timebuf, sizeof timebuf, "%a, %d %b %Y %H:%M:%S %Z", std::gmtime(&rawtime.tv_sec));

                    std::string range = conn_obj.header("range");
                    if (range.empty()) {
                        conn_obj.header("Content-Type", actual_mimetype);
                        conn_obj.header("Cache-Control", "public, must-revalidate, max-age=0");
                        conn_obj.header("Pragma", "no-cache");
                        conn_obj.header("Accept-Ranges", "bytes");
                        conn_obj.header("Content-Length", std::to_string(fsize));
                        conn_obj.header("Last-Modified", timebuf);
                        conn_obj.header("Content-Transfer-Encoding: binary");
                        conn_obj.sendFile(infile);
                    }
                    else {
                        //
                        // now we parse the range
                        //
                        std::regex re("bytes=\\s*(\\d+)-(\\d*)[\\D.*]?");
                        std::cmatch m;
                        int start = 0; // lets assume beginning of file
                        int end = fsize - 1; // lets assume whole file
                        if (std::regex_match (range.c_str(), m, re)) {
                            if (m.size() < 2) {
                                throw Error(__file__, __LINE__, "Range expression invalid!");
                            }
                            start = std::stoi(m[1]);
                            if ((m.size() > 1) && !m[2].str().empty()) {
                                end = std::stoi(m[2]);
                            }
                        }
                        else {
                            throw Error(__file__, __LINE__, "Range expression invalid!");
                        }

                        conn_obj.status(Connection::PARTIAL_CONTENT);
                        conn_obj.header("Content-Type", actual_mimetype);
                        conn_obj.header("Cache-Control", "public, must-revalidate, max-age=0");
                        conn_obj.header("Pragma", "no-cache");
                        conn_obj.header("Accept-Ranges", "bytes");
                        conn_obj.header("Content-Length", std::to_string(end - start + 1));
                        conn_obj.header("Content-Length", std::to_string(end - start + 1));
                        std::stringstream ss;
                        ss << "bytes " << start << "-" << end << "/" << fsize;
                        conn_obj.header("Content-Range", ss.str());
                        conn_obj.header("Content-Disposition",  std::string("inline; filename=") + urldecode(params[iiif_identifier]));
                        conn_obj.header("Content-Transfer-Encoding: binary");
                        conn_obj.header("Last-Modified", timebuf);
                        conn_obj.sendFile(infile, 8192, start, end);
                    }
                    conn_obj.flush();
                    return;
                }
            } else {
                syslog(LOG_WARNING, "GET: %s not accessible", infile.c_str());
                send_error(conn_obj, Connection::NOT_FOUND);
                conn_obj.flush();
                return;
            }
        }

        //
        // test if there are enough parameters to fullfill the info request
        //
        if (params.size() < 3) {
            send_error(conn_obj, Connection::BAD_REQUEST, "Query has too few parameters");
            return;
        }

        //string prefix = urldecode(params[iiif_prefix]);
        //string identifier = urldecode(params[iiif_identifier]);

        //
        // we have a request for the info json
        //
        if (params[iiif_region] == "info.json") {
            iiif_send_info(conn_obj, serv, luaserver, params, prefix_as_path);
            return;
        }

        if (params[iiif_region] == "knora.json") {
            knora_send_info(conn_obj, serv, luaserver, params, prefix_as_path);
            return;
        }

        if (params.size() < 7) {
            send_error(conn_obj, Connection::BAD_REQUEST, "Query has too few parameters");
            return;
        }

        if (params.size() > 7) {
            send_error(conn_obj, Connection::NOT_FOUND, "Too many \"/\"'s – imageid not found");
            return;
        }

        //
        // getting the identifier (which in case of a PDF or multipage TIFF my contain a page id (identifier@pagenum)
        SipiIdentifier sid = SipiIdentifier(params[iiif_identifier]);

        //
        // getting region parameters
        //
        auto region = std::make_shared<SipiRegion>();

        try {
            region = std::make_shared<SipiRegion>(params[iiif_region]);
            std::stringstream ss;
            ss << *region;
            syslog(LOG_DEBUG, "%s", ss.str().c_str());
        } catch (Sipi::SipiError &err) {
            send_error(conn_obj, Connection::BAD_REQUEST, err);
            return;
        }

        //
        // getting scaling/size parameters
        //

        auto size = std::make_shared<SipiSize>();

        try {
            size = std::make_shared<SipiSize>(params[iiif_size]);
            std::stringstream ss;
            ss << *size;
            syslog(LOG_DEBUG, "%s", ss.str().c_str());
        } catch (Sipi::SipiError &err) {
            send_error(conn_obj, Connection::BAD_REQUEST, err);
            return;
        }

        //
        // getting rotation parameters
        //

        SipiRotation rotation;

        try {
            rotation = SipiRotation(params[iiif_rotation]);
            std::stringstream ss;
            ss << rotation;
            syslog(LOG_DEBUG, "%s", ss.str().c_str());
        } catch (Sipi::SipiError &err) {
            send_error(conn_obj, Connection::BAD_REQUEST, err);
            return;
        }

        SipiQualityFormat quality_format;
        try {
            quality_format = SipiQualityFormat(params[iiif_qualityformat]);
            std::stringstream ss;
            ss << quality_format;
            syslog(LOG_DEBUG, "%s", ss.str().c_str());
        } catch (Sipi::SipiError &err) {
            send_error(conn_obj, Connection::BAD_REQUEST, err);
            return;
        }

        //
        // build the filename and test if the file exists and is readable
        //
        /*
        string infile;

        if (params[iiif_prefix] == serv->salsah_prefix()) {
            Salsah salsah;

            try {
                salsah = Salsah(&conn_obj, urldecode(params[iiif_identifier]));
            } catch (Sipi::SipiError &err) {
                send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }

            infile = salsah.getFilepath();

            if (salsah.getRights() < Salsah::RESOURCE_ACCESS_VIEW_RESTRICTED) {
                send_error(conn_obj, Connection::FORBIDDEN, "No right to view image");
                return;
            }
        } else {
            infile = serv->imgroot() + "/" + prefix + "/" + identifier;
        }
        */

        //
        // here we start the lua script which checks for permissions
        //
        std::string infile;  // path to the input file on the server
        std::string permission; // the permission string
        std::string watermark; // path to watermark file, or empty, if no watermark required
        auto restriction_size = std::make_shared<SipiSize>(); // size of restricted image... (SizeType::FULL if unrestricted)

        if (luaserver.luaFunctionExists(pre_flight_func_name)) {
            std::pair<std::string, std::string> pre_flight_return_values;

            try {
                pre_flight_return_values = call_pre_flight(conn_obj, luaserver, urldecode(params[iiif_prefix]), sid.getIdentifier());
            } catch (SipiError &err) {
                send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }

            permission = pre_flight_return_values.first;
            infile = pre_flight_return_values.second;

            //
            // here we adjust the path for the subdirs
            //
            if (SipiFilenameHash::getLevels() > 0) {
                bool use_subdirs = true;
                if (prefix_as_path) {
                    for (auto str: serv->dirs_to_exclude()) {
                        if (str == urldecode(params[iiif_prefix])) {
                            use_subdirs = false; // prefix is in list which is excluded from usiong subdirs
                        }
                    }
                }
                if (use_subdirs) {
                    size_t ppos = infile.rfind("/");
                    if ((ppos != std::string::npos) && (ppos < (infile.size() - 1))) {
                        std::string dirpart = infile.substr(0, ppos);
                        std::string filepart = infile.substr(ppos + 1);
                        SipiFilenameHash identifier = SipiFilenameHash(filepart);
                        infile = dirpart + "/" + identifier.filepath();
                    }
                }
            }

            size_t colon_pos = permission.find(':');
            std::string qualifier;

            if (colon_pos != std::string::npos) {
                qualifier = permission.substr(colon_pos + 1);
                permission = permission.substr(0, colon_pos);
            }

            if (permission != "allow") {
                if (permission == "restrict") {
                    colon_pos = qualifier.find('=');
                    std::string restriction_type = qualifier.substr(0, colon_pos);
                    std::string restriction_param = qualifier.substr(colon_pos + 1);
                    if (restriction_type == "watermark") {
                        watermark = restriction_param;
                    } else if (restriction_type == "size") {
                        restriction_size = std::make_shared<SipiSize>(restriction_param);
                    } else {
                        send_error(conn_obj, Connection::UNAUTHORIZED, "Unauthorized access");
                        return;
                    }
                } else {
                    send_error(conn_obj, Connection::UNAUTHORIZED, "Unauthorized access");
                    return;
                }
            }
        } else {
            SipiFilenameHash identifier = SipiFilenameHash(sid.getIdentifier());
            if (prefix_as_path) {
                bool use_subdirs = true;
                for (auto str: serv->dirs_to_exclude()) {
                    if (str == urldecode(params[iiif_prefix])) {
                        use_subdirs = false; // prefix is in list which is excluded from usiong subdirs
                    }
                }
                if (use_subdirs) {
                    infile = serv->imgroot() + "/" + urldecode(params[iiif_prefix]) + "/" +
                             identifier.filepath();
                }
                else {
                    infile = serv->imgroot() + "/" + urldecode(params[iiif_prefix]) + "/" +
                            urldecode(params[iiif_identifier]);
                }
            } else {
                infile = serv->imgroot() + "/" + identifier.filepath();
            }
        }

        //
        // determine the extension of the file in the SIPI repo
        //
        SipiQualityFormat::FormatType in_format = SipiQualityFormat::UNSUPPORTED;

        std::string actual_mimetype = shttps::Parsing::getFileMimetype(infile).first;
        syslog(LOG_DEBUG, "MIMETYPE= %s: file %s", actual_mimetype.c_str(), infile.c_str());

        if (actual_mimetype == "image/tiff") in_format = SipiQualityFormat::TIF;
        if (actual_mimetype == "image/jpeg") in_format = SipiQualityFormat::JPG;
        if (actual_mimetype == "image/png") in_format = SipiQualityFormat::PNG;
        if ((actual_mimetype == "image/jpx") || (actual_mimetype == "image/jp2")) in_format = SipiQualityFormat::JP2;
        if (actual_mimetype == "application/pdf") in_format = SipiQualityFormat::PDF;

        if (access(infile.c_str(), R_OK) != 0) { // test, if file exists
            syslog(LOG_ERR, "File %s not found", infile.c_str());
            send_error(conn_obj, Connection::NOT_FOUND);
            return;
        }

        float angle;
        bool mirror = rotation.get_rotation(angle);

        //
        // get cache info
        //
        std::shared_ptr<SipiCache> cache = serv->cache();
        size_t img_w = 0, img_h = 0;
        int numpages = 0;
        int pagenum = sid.getPage();

        if ((in_format == SipiQualityFormat::PDF) && (pagenum < 1)) {
            if (size->getType() != SipiSize::FULL) {
                send_error(conn_obj, Connection::BAD_REQUEST, "PDF must have size qualifier of \"full\"");
                return;
            }

            if (region->getType() != SipiRegion::FULL) {
                send_error(conn_obj, Connection::BAD_REQUEST, "PDF must have region qualifier of \"full\"");
                return;
            }

            float rot;

            if (rotation.get_rotation(rot) || (rot != 0.0)) {
                send_error(conn_obj, Connection::BAD_REQUEST, "PDF must have rotation qualifier of \"0\"");
                return;
            }

            if ((quality_format.quality() != SipiQualityFormat::DEFAULT) ||
                (quality_format.format() != SipiQualityFormat::PDF)) {
                send_error(conn_obj, Connection::BAD_REQUEST, "PDF must have quality qualifier of \"default.pdf\"");
                return;
            }
        } else {
            //
            // get image dimensions, needed for get_canonical...
            //
            if ((cache == nullptr) || !cache->getSize(infile, img_w, img_h, numpages)) {
                Sipi::SipiImage tmpimg;
                Sipi::SipiImgInfo info;
                try {
                    info = tmpimg.getDim(infile, pagenum);
                } catch (SipiImageError &err) {
                    send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err.to_string());
                    return;
                }
                if (info.success == SipiImgInfo::FAILURE) {
                    send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, "Couldn't get image dimensions!");
                    return;
                }
                img_w = info.width;
                img_h = info.height;
                numpages = info.numpages;
            }

            size_t tmp_r_w, tmp_r_h;
            int tmp_red;
            bool tmp_ro;
            size->get_size(img_w, img_h, tmp_r_w, tmp_r_h, tmp_red, tmp_ro);
            restriction_size->get_size(img_w, img_h, tmp_r_w, tmp_r_h, tmp_red, tmp_ro);

            if (*size > *restriction_size) {
                size = restriction_size;
            }
        }

        //.....................................................................
        // here we start building the canonical URL
        //
        std::pair<std::string, std::string> tmppair;

        try {
            tmppair = serv->get_canonical_url(img_w, img_h, conn_obj.host(), params[iiif_prefix],
                                              sid.getIdentifier(), region, size, rotation, quality_format, sid.getPage());
        } catch (Sipi::SipiError &err) {
            send_error(conn_obj, Connection::BAD_REQUEST, err);
            return;
        }

        std::string canonical_header = tmppair.first;
        std::string canonical = tmppair.second;

        //
        // now we check if we can send the file directly
        //
        if ((region->getType() == SipiRegion::FULL) && (size->getType() == SipiSize::FULL) && (angle == 0.0) &&
            (!mirror) && watermark.empty() && (quality_format.format() == in_format) &&
            (quality_format.quality() == SipiQualityFormat::DEFAULT) && (sid.getPage() < 1)) {

            syslog(LOG_DEBUG, "Sending unmodified file....");
            conn_obj.status(Connection::OK);
            conn_obj.header("Cache-Control", "must-revalidate, post-check=0, pre-check=0");
            conn_obj.header("Link", canonical_header);

            switch (quality_format.format()) {
                case SipiQualityFormat::TIF: {
                    conn_obj.header("Content-Type", "image/tiff"); // set the header (mimetype)
                    break;
                }

                case SipiQualityFormat::JPG: {
                    conn_obj.header("Content-Type", "image/jpeg"); // set the header (mimetype)
                    break;
                }

                case SipiQualityFormat::PNG: {
                    conn_obj.header("Content-Type", "image/png"); // set the header (mimetype)
                    break;
                }

                case SipiQualityFormat::JP2: {
                    conn_obj.header("Content-Type", "image/jp2"); // set the header (mimetype)
                    break;
                }

                case SipiQualityFormat::PDF: {
                    conn_obj.header("Content-Type", "application/pdf"); // set the header (mimetype)
                    break;
                }

                default: {
                }
            }

            try {
                syslog(LOG_INFO, "Sending file %s", infile.c_str());
                conn_obj.sendFile(infile);
            } catch (shttps::InputFailure iofail) {
                // -1 was thrown
                syslog(LOG_WARNING, "Browser unexpectedly closed connection");
                return;
            } catch (Sipi::SipiError &err) {
                send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }

            return;
        } // finish sending unmodified file in toto

        /*
        if (quality_format.format() == SipiQualityFormat::PDF) {
            send_error(conn_obj, Connection::BAD_REQUEST, "Conversion to PDF not yet supported");
        }
        */

        syslog(LOG_DEBUG, "Checking for cache...");

        if (cache != nullptr) {
            syslog(LOG_DEBUG, "Cache found, testing for canonical %s", canonical.c_str());
            std::string cachefile = cache->check(infile, canonical);

            if (!cachefile.empty()) {
                syslog(LOG_DEBUG, "Using cachefile %s", cachefile.c_str());
                conn_obj.status(Connection::OK);
                conn_obj.header("Cache-Control", "must-revalidate, post-check=0, pre-check=0");
                conn_obj.header("Link", canonical_header);

                switch (quality_format.format()) {
                    case SipiQualityFormat::TIF: {
                        conn_obj.header("Content-Type", "image/tiff"); // set the header (mimetype)
                        break;
                    }

                    case SipiQualityFormat::JPG: {
                        conn_obj.header("Content-Type", "image/jpeg"); // set the header (mimetype)
                        break;
                    }

                    case SipiQualityFormat::PNG: {
                        conn_obj.header("Content-Type", "image/png"); // set the header (mimetype)
                        break;
                    }

                    case SipiQualityFormat::JP2: {
                        conn_obj.header("Content-Type", "image/jp2"); // set the header (mimetype)
                        break;
                    }

                    case SipiQualityFormat::PDF: {
                        conn_obj.header("Content-Type", "application/pdf"); // set the header (mimetype)
                        break;
                    }

                    default: {
                    }
                }

                try {
                    syslog(LOG_DEBUG, "Sending cachefile %s", cachefile.c_str());
                    conn_obj.sendFile(cachefile);
                } catch (shttps::InputFailure err) {
                    // -1 was thrown
                    syslog(LOG_WARNING, "Browser unexpectedly closed connection");
                    return;
                } catch (Sipi::SipiError &err) {
                    send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err);
                    return;
                }

                return;
            }
        }

        syslog(LOG_WARNING, "Nothing found in cache, reading and transforming file...");
        Sipi::SipiImage img;

        try {
            img.read(infile, sid.getPage(), region, size, quality_format.format() == SipiQualityFormat::JPG, serv->scaling_quality());
        } catch (const SipiImageError &err) {
            send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err.to_string());
            return;
        }

        //
        // now we rotate
        //
        if (mirror || (angle != 0.0)) {
            try {
                img.rotate(angle, mirror);
            } catch (Sipi::SipiError &err) {
                send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }
        }

        if (quality_format.quality() != SipiQualityFormat::DEFAULT) {
            switch (quality_format.quality()) {
                case SipiQualityFormat::COLOR: {
                    img.convertToIcc(SipiIcc(icc_sRGB), 8); // for now, force 8 bit/sample
                    break;
                }

                case SipiQualityFormat::GRAY: {
                    img.convertToIcc(SipiIcc(icc_GRAY_D50), 8); // for now, force 8 bit/sample
                    break;
                }

                case SipiQualityFormat::BITONAL: {
                    img.toBitonal();
                    break;
                }

                default: {
                    send_error(conn_obj, Connection::BAD_REQUEST, "Invalid quality specificer");
                    return;
                }
            }
        }

        //
        // let's add a watermark if necessary
        //
        if (!watermark.empty()) {
            watermark = "watermark.tif";

            try {
                img.add_watermark(watermark);
            } catch (Sipi::SipiError &err) {
                send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }

            syslog(LOG_INFO, "GET %s: adding watermark", uri.c_str());
        }

        img.connection(&conn_obj);
        conn_obj.header("Cache-Control", "must-revalidate, post-check=0, pre-check=0");
        std::string cachefile;

        if (cache != nullptr) {
            cachefile = cache->getNewCacheFileName();
            syslog(LOG_INFO, "Writing new cache file %s", cachefile.c_str());
        }

        try {
            bool cache_open = false;
            switch (quality_format.format()) {
                case SipiQualityFormat::JPG: {
                    conn_obj.status(Connection::OK);
                    conn_obj.header("Link", canonical_header);
                    conn_obj.header("Content-Type", "image/jpeg"); // set the header (mimetype)

                    if ((img.getNc() > 3) && (img.getNalpha() > 0)) { // we have an alpha channel....
                        for (size_t i = 3; i < (img.getNalpha() + 3); i++) img.removeChan(i);
                    }

                    Sipi::SipiIcc icc = Sipi::SipiIcc(Sipi::icc_sRGB); // force sRGB !!
                    img.convertToIcc(icc, 8);
                    conn_obj.setChunkedTransfer();

                    if (cache != nullptr) {
                        conn_obj.openCacheFile(cachefile);
                        cache_open = true;
                    }

                    syslog(LOG_DEBUG, "Before writing JPG...");

                    try {
                        img.write("jpg", "HTTP", serv->jpeg_quality());
                    } catch (SipiImageError &err) {
                        syslog(LOG_ERR, "%s", err.to_string().c_str());

                        if (cache != nullptr) {
                            conn_obj.closeCacheFile();
                            cache_open = false;
                            unlink(cachefile.c_str());
                        }

                        break;
                    }
                    syslog(LOG_DEBUG, "After writing JPG...");
                    break;
                }

                case SipiQualityFormat::JP2: {
                    conn_obj.status(Connection::OK);
                    conn_obj.header("Link", canonical_header);
                    conn_obj.header("Content-Type", "image/jp2"); // set the header (mimetype)
                    conn_obj.setChunkedTransfer();

                    if (cache != nullptr) {
                        conn_obj.openCacheFile(cachefile);
                        cache_open = true;
                    }

                    syslog(LOG_DEBUG, "Before writing J2K...");

                    try {
                        img.write("jpx", "HTTP");
                    } catch (SipiImageError &err) {
                        syslog(LOG_ERR, "%s", err.to_string().c_str());

                        if (cache != nullptr) {
                            conn_obj.closeCacheFile();
                            cache_open = false;
                            unlink(cachefile.c_str());
                        }
                    }
                    syslog(LOG_DEBUG, "After writing J2K...");
                    break;
                }

                case SipiQualityFormat::TIF: {
                    conn_obj.status(Connection::OK);
                    conn_obj.header("Link", canonical_header);
                    conn_obj.header("Content-Type", "image/tiff"); // set the header (mimetype)
                    // no chunked transfer needed...

                    if (cache != nullptr) {
                        conn_obj.openCacheFile(cachefile);
                        cache_open = true;
                    }

                    syslog(LOG_DEBUG, "Before writing TIF...");

                    try {
                        img.write("tif", "HTTP");
                    } catch (SipiImageError &err) {
                        syslog(LOG_ERR, "%s", err.to_string().c_str());

                        if (cache != nullptr) {
                            conn_obj.closeCacheFile();
                            cache_open = false;
                            unlink(cachefile.c_str());
                        }
                        break;
                    }
                    syslog(LOG_DEBUG, "After writing TIF...");
                    break;
                }

                case SipiQualityFormat::PNG: {
                    conn_obj.status(Connection::OK);
                    conn_obj.header("Link", canonical_header);
                    conn_obj.header("Content-Type", "image/png"); // set the header (mimetype)
                    conn_obj.setChunkedTransfer();

                    if (cache != nullptr) {
                        conn_obj.openCacheFile(cachefile);
                        cache_open = true;
                    }

                    syslog(LOG_DEBUG, "Before writing PNG...");

                    try {
                        img.write("png", "HTTP");
                    } catch (SipiImageError &err) {
                        syslog(LOG_ERR, "%s", err.to_string().c_str());

                        if (cache != nullptr) {
                            conn_obj.closeCacheFile();
                            cache_open = false;
                            unlink(cachefile.c_str());
                        }
                        break;
                    }
                    syslog(LOG_DEBUG, "After writing PNG...");
                    break;
                }

                case SipiQualityFormat::PDF: {
                    conn_obj.status(Connection::OK);
                    conn_obj.header("Link", canonical_header);
                    conn_obj.header("Content-Type", "application/pdf"); // set the header (mimetype)
                    conn_obj.setChunkedTransfer();

                    if (cache != nullptr) {
                        conn_obj.openCacheFile(cachefile);
                        cache_open = true;
                    }
                    syslog(LOG_DEBUG, "Before writing PDF...");
                    try {
                        img.write("pdf", "HTTP");
                    } catch (SipiImageError &err) {
                        syslog(LOG_ERR, "%s", err.to_string().c_str());

                        if (cache != nullptr) {
                            conn_obj.closeCacheFile();
                            cache_open = false;
                            unlink(cachefile.c_str());
                        }
                        break;
                    }
                    syslog(LOG_DEBUG, "After writing PDF...");
                   break;
                }

                default: {
                    // HTTP 400 (format not supported)
                    syslog(LOG_WARNING, "Unsupported file format requested! Supported are .jpg, .jp2, .tif, .png");
                    conn_obj.setBuffer();
                    conn_obj.status(Connection::BAD_REQUEST);
                    conn_obj.header("Content-Type", "text/plain");
                    conn_obj << "Not Implemented!\n";
                    conn_obj << "Unsupported file format requested! Supported are .jpg, .jp2, .tif, .png\n";
                    conn_obj.flush();
                }
            }

            if (cache_open) {
                conn_obj.closeCacheFile();
                syslog(LOG_DEBUG, "Adding cachefile %s to internal list", cachefile.c_str());
                cache->add(infile, canonical, cachefile, img_w, img_h, numpages);
            }

        } catch (Sipi::SipiError &err) {
            send_error(conn_obj, Connection::INTERNAL_SERVER_ERROR, err);
            return;
        }

        conn_obj.flush();
        syslog(LOG_INFO, "GET %s: file %s", uri.c_str(), infile.c_str());
        return;
    }
    //=========================================================================


    static void favicon_handler(Connection &conn_obj, shttps::LuaServer &luaserver, void *user_data, void *dummy) {
        conn_obj.status(Connection::OK);
        conn_obj.header("Content-Type", "image/x-icon");
        conn_obj.send(favicon_ico, favicon_ico_len);
    }
    //=========================================================================

    static void test_handler(Connection &conn_obj, shttps::LuaServer &luaserver, void *user_data, void *dummy) {
        lua_State *L = luaL_newstate();
        luaL_openlibs(L);

        conn_obj.status(Connection::OK);
        conn_obj.header("Content-Type", "text/plain");
        conn_obj << "TEST test TEST test TEST!\n";

        lua_close(L);
    }
    //=========================================================================

    static void exit_handler(Connection &conn_obj, shttps::LuaServer &luaserver, void *user_data, void *dummy) {
        syslog(LOG_INFO, "Exit handler called. Stopping SIPI");
        conn_obj.status(Connection::OK);
        conn_obj.header("Content-Type", "text/plain");
        conn_obj << "Stopping Sipi\n";
        conn_obj.server()->stop();
    }
    //=========================================================================

    SipiHttpServer::SipiHttpServer(int port_p, unsigned nthreads_p, const std::string userid_str,
                                   const std::string &logfile_p, const std::string &loglevel_p) : Server::Server(port_p,
                                                                                                                 nthreads_p,
                                                                                                                 userid_str,
                                                                                                                 logfile_p,
                                                                                                                 loglevel_p) {
        _salsah_prefix = "imgrep";
        _cache = nullptr;
        _scaling_quality = {HIGH, HIGH, HIGH, HIGH};
    }
    //=========================================================================

    void SipiHttpServer::cache(const std::string &cachedir_p, long long max_cachesize_p, unsigned max_nfiles_p,
                               float cache_hysteresis_p) {
        try {
            _cache = std::make_shared<SipiCache>(cachedir_p, max_cachesize_p, max_nfiles_p, cache_hysteresis_p);
        } catch (const SipiError &err) {
            _cache = nullptr;
            syslog(LOG_WARNING, "Couldn't open cache directory %s: %s", cachedir_p.c_str(), err.to_string().c_str());
        }
    }
    //=========================================================================

    void SipiHttpServer::run(void) {

        int old_ll = setlogmask(LOG_MASK(LOG_INFO));
        syslog(LOG_INFO, "Sipi server starting");
        //
        // setting the image root
        //
        syslog(LOG_INFO, "Serving images from %s", _imgroot.c_str());
        syslog(LOG_DEBUG, "Salsah prefix: %s", _salsah_prefix.c_str());
        setlogmask(old_ll);

        addRoute(Connection::GET, "/favicon.ico", favicon_handler);
        addRoute(Connection::GET, "/", process_get_request);
        addRoute(Connection::GET, "/admin/test", test_handler);
        addRoute(Connection::GET, "/admin/exit", exit_handler);

        user_data(this);

        Server::run();
    }
    //=========================================================================

}
