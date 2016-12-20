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
#include <assert.h>
#include <stdlib.h>
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "SipiImage.h"
#include "SipiError.h"
#include "iiifparser/SipiQualityFormat.h"
#include "PhpSession.h"
// #include "Salsah.h"

#include "shttps/Global.h"
#include "SipiHttpServer.h"
#include "shttps/Connection.h"

#include "jansson.h"
#include "favicon.h"

#include "shttps/Logger.h"  // logging...

#include "lua.hpp"

using namespace std;
using namespace shttps;
static const char __file__[] = __FILE__;

namespace Sipi {

    typedef enum {
        iiif_prefix = 0,			//!< http://{url}/*{prefix}*/{id}/{region}/{size}/{rotation}/{quality}.{format}
        iiif_identifier = 1,		//!< http://{url}/{prefix}/*{id}*/{region}/{size}/{rotation}/{quality}.{format}
        iiif_region = 2,			//!< http://{url}/{prefix}/{id}/{region}/{size}/{rotation}/{quality}.{format}
        iiif_size = 3,				//!< http://{url}/{prefix}/{id}/{region}/*{size}*/{rotation}/{quality}.{format}
        iiif_rotation = 4,			//!< http://{url}/{prefix}/{id}/{region}/{size}/*{rotation}*/{quality}.{format}
        iiif_qualityformat = 5,		//!< http://{url}/{prefix}/{id}/{region}/{size}/{rotation}/*{quality}.{format}*
    } IiifParams;


    static void send_error(Connection &conobj, Connection::StatusCodes code, const string &errmsg) {
        auto logger = Logger::getLogger(shttps::loggername);
        conobj.status(code);
        conobj.setBuffer();
        conobj.header("Content-Type", "text/plain");
        *logger << Logger::LogLevel::ERROR << "GET: " << conobj.uri() << " failed: ";
        switch (code) {
            case Connection::BAD_REQUEST:
                conobj << "Bad Request!";
                *logger << "Bad Request! " << errmsg << Logger::LogAction::FLUSH;
                break;
            case Connection::FORBIDDEN:
                conobj << "Forbidden!";
                *logger << "Forbidden! " << errmsg << Logger::LogAction::FLUSH;
                break;
            case Connection::NOT_FOUND:
                conobj << "Not Found!";
                *logger << "Not Found! " << errmsg << Logger::LogAction::FLUSH;
                break;
            case Connection::INTERNAL_SERVER_ERROR:
                conobj << "Internal Server Error!";
                *logger << "Internal Server Error! " << errmsg << Logger::LogAction::FLUSH;
                break;
            case Connection::NOT_IMPLEMENTED:
                conobj << "Not Implemented!";
                *logger << "Not Implemented! " << errmsg << Logger::LogAction::FLUSH;
                break;
            case Connection::SERVICE_UNAVAILABLE:
                conobj << "Service Unavailable!";
                *logger << "Service Unavailable! " << errmsg << Logger::LogAction::FLUSH;
                break;
            default:
                *logger << "Unknown error! " << errmsg << Logger::LogAction::FLUSH;
                break; // do nothing
        }
        conobj << errmsg;
        conobj.flush();
    }
    //=========================================================================


    static void send_error(Connection &conobj, Connection::StatusCodes code, const SipiError &err) {
        auto logger = Logger::getLogger(shttps::loggername);
        conobj.status(code);
        conobj.setBuffer();
        conobj.header("Content-Type", "text/plain");
        stringstream outss;
        outss << err;
        *logger << Logger::LogLevel::ERROR << "GET: " << conobj.uri() << " failed: ";
        switch (code) {
            case Connection::BAD_REQUEST:
                conobj << "Bad Request!";
                *logger << "Bad Request! " << outss.str() << Logger::LogAction::FLUSH;
                break;
            case Connection::FORBIDDEN:
                conobj << "Forbidden!";
                *logger << "Forbidden! " << outss.str() << Logger::LogAction::FLUSH;
                break;
            case Connection::NOT_FOUND:
                conobj << "Not Found!";
                *logger << "Not Found! " << outss.str() << Logger::LogAction::FLUSH;
                break;
            case Connection::INTERNAL_SERVER_ERROR:
                conobj << "Internal Server Error!";
                *logger << "Internal Server Error! " << outss.str() << Logger::LogAction::FLUSH;
                break;
            case Connection::NOT_IMPLEMENTED:
                conobj << "Not Implemented!";
                *logger << "Not Implemented! " << outss.str() << Logger::LogAction::FLUSH;
                break;
            case Connection::SERVICE_UNAVAILABLE:
                conobj << "Service Unavailable!";
                *logger << "Service Unavailable! " << outss.str() << Logger::LogAction::FLUSH;
                break;
            default:
                *logger << "Unknown error! " << outss.str() << Logger::LogAction::FLUSH;
                break; // do nothing
        }
        conobj << outss.str();
        conobj.flush();
    }
    //=========================================================================


    static void send_error(Connection &conobj, Connection::StatusCodes code) {
        auto logger = Logger::getLogger(shttps::loggername);
        conobj.status(code);
        conobj.setBuffer();
        conobj.header("Content-Type", "text/plain");
        *logger << Logger::LogLevel::ERROR << "GET: " << conobj.uri() << " failed: ";
        switch (code) {
            case Connection::BAD_REQUEST:
                conobj << "Bad Request!";
                *logger << "Bad Request!" << Logger::LogAction::FLUSH;
                break;
            case Connection::NOT_FOUND:
                conobj << "Not Found!";
                *logger << "Not Found!" << Logger::LogAction::FLUSH;
                break;
            case Connection::INTERNAL_SERVER_ERROR:
                conobj << "Internal Server Error!";
                *logger << "Internal Server Error!" << Logger::LogAction::FLUSH;
                break;
            case Connection::NOT_IMPLEMENTED:
                conobj << "Not Implemented!";
                *logger << "Not Implemented!" << Logger::LogAction::FLUSH;
                break;
            case Connection::SERVICE_UNAVAILABLE:
                conobj << "Service Unavailable!";
                *logger << "Service Unavailable!" << Logger::LogAction::FLUSH;
                break;
            default:
                *logger << "Unknown error!" << Logger::LogAction::FLUSH;
                break; // do nothing
        }
        conobj.flush();
    }
    //=========================================================================



    static void iiif_send_info(Connection &conobj, SipiHttpServer *serv, shttps::LuaServer &luaserver, vector<string> &params, const string &imgroot, bool prefix_as_path) {
        auto logger = Logger::getLogger(shttps::loggername);
        conobj.setBuffer(); // we want buffered output, since we send JSON text...
        const string contenttype = conobj.header("accept");

        conobj.header("Access-Control-Allow-Origin", "*");
        /*
        string infile; // path to file to convert and serve
        if (params[iiif_prefix] == salsah_prefix) {

            Salsah salsah;
            try {
                salsah = Salsah(&conobj, params[iiif_identifier]);
            }
            catch (Sipi::SipiError &err) {
                send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }

            infile = salsah.getFilepath();

            if (salsah.getRights() < Salsah::RESOURCE_ACCESS_VIEW_RESTRICTED) {
                send_error(conobj, Connection::FORBIDDEN, "No right to view image!");
                return;
            }
        }
        else {
            infile = imgroot + "/" + params[iiif_prefix] + "/" + params[iiif_identifier];
        }
        */

        string infile;
        string permission;
        //
        // here we start the lua script which checks for permissions
        //
        const string funcname = "pre_flight";
        if (luaserver.luaFunctionExists(&funcname)) {
            LuaValstruct lval[3];
            lval[0].type = LuaValstruct::STRING_TYPE;
            lval[0].value.s = urldecode(params[iiif_prefix]);

            lval[1].type = LuaValstruct::STRING_TYPE;
            lval[1].value.s = urldecode(params[iiif_identifier]);

            string cookie = conobj.header("cookie");
            lval[2].type = LuaValstruct::STRING_TYPE;
            lval[2].value.s = cookie.c_str();

            vector<LuaValstruct> rval;
            try {
                rval = luaserver.executeLuafunction(&funcname, 3, lval);
            }
            catch (shttps::Error err) {
                send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err.to_string());
                return;
            }

            if (rval[0].type == LuaValstruct::STRING_TYPE) {
                permission = rval[0].value.s;
            }
            else {
                send_error(conobj, Connection::INTERNAL_SERVER_ERROR, "Lua function pre_flight must return two strings");
                return;
            }

            if (rval[1].type == LuaValstruct::STRING_TYPE) {
                infile = rval[1].value.s;
            }
            else {
                send_error(conobj, Connection::INTERNAL_SERVER_ERROR, "Lua function pre_flight must return two strings");
                return;
            }

            size_t pos = permission.find('=');
            string qualifier;
            if (pos != string::npos) {
                qualifier = permission.substr(pos + 1);
                permission = permission.substr(0, pos);
            }
            if (permission != "allow") {
                if (permission == "restricted") {
                    cerr << "Qualifier=" << qualifier << endl;
                }
                else {
                    send_error(conobj, Connection::UNAUTHORIZED, "Unauthorized access!");
                    return;
                }
            }
        }
        else {
            if (prefix_as_path) {
                infile = serv->imgroot() + "/" + urldecode(params[iiif_prefix]) + "/" +
                         urldecode(params[iiif_identifier]);
            }
            else {
                infile = serv->imgroot() + "/" + urldecode(params[iiif_identifier]);
            }
        }

        //
        // test if we have access to the file
        //
        if (access(infile.c_str(), R_OK) != 0) { // test, if file exists
            send_error(conobj, Connection::BAD_REQUEST, "File not readable!");
            return;
        }
        if (!contenttype.empty() && (contenttype == "application/ld+json")) {
            conobj.header("Content-Type", "application/ld+json");
        }
        else {
            conobj.header("Content-Type", "application/json");
            conobj.header("Link", "<http://iiif.io/api/image/2/context.json>; rel=\"http://www.w3.org/ns/json-ld#context\"; type=\"application/ld+json\"");
        }
        json_t *root = json_object();

        json_object_set_new(root, "@context", json_string("http://iiif.io/api/image/2/context.json"));

        std::string host = conobj.header("host");
        std::string id = std::string("http://") + host + "/" + params[iiif_prefix] + "/" + params[iiif_identifier]; //// ?????????????????????????????????????
        json_object_set_new(root, "@id", json_string(id.c_str()));

        json_object_set_new(root, "protocol", json_string("http://iiif.io/api/image"));

        int width, height;
        //
        // get cache info
        //
        SipiCache *cache = serv->cache();
        if ((cache == NULL) || !cache->getSize(infile, width, height)) {
            Sipi::SipiImage tmpimg;
            try {
                tmpimg.getDim(infile, width, height);
            }
            catch(SipiImageError &err) {
                send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err.get_error());
                return;
            }
        }

        json_object_set_new(root, "width", json_integer(width));
        json_object_set_new(root, "height", json_integer(height));

        json_t *sizes = json_array();
        for (int i = 1; i < 5; i++) {
            SipiSize size(i);
            int w, h, r;
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

        const char * formats_str[] = {"tif", "jpg", "png", "jp2"};
        json_t *formats = json_array();
        for (int i = 0; i < sizeof(formats_str)/sizeof(char*); i++) {
            json_array_append_new(formats, json_string(formats_str[i]));
        }
        json_object_set_new(profile, "formats", formats);

        const char *qualities_str[] = {"color", "gray"};
        json_t *qualities = json_array();
        for (int i = 0; i < sizeof(qualities_str)/sizeof(char*); i++) {
            json_array_append_new(qualities, json_string(qualities_str[i]));
        }
        json_object_set_new(profile, "qualities", qualities);

        const char * supports_str[] = {
            "color",
            "cors",
            "mirroring",
            "profileLinkHeader",
            "regionByPct",
            "regionByPx",
            "rotationArbitrary",
            "rotationBy90s",
            "sizeAboveFull",
            "sizeByWhListed",
            "sizeByForcedWh",
            "sizeByH",
            "sizeByPct",
            "sizeByW",
            "sizeByWh"
        };
        json_t *supports = json_array();
        for (int i = 0; i < sizeof(supports_str)/sizeof(char*); i++) {
            json_array_append_new(supports, json_string(supports_str[i]));
        }
        json_object_set_new(profile, "supports", supports);

        json_array_append_new(profile_arr, profile);

        json_object_set_new(root, "profile", profile_arr);

        char *json_str = json_dumps(root, JSON_INDENT(3));

        conobj.sendAndFlush(json_str, strlen(json_str));

        free (json_str);

        //TODO and all the other CJSON obj?
        json_decref(root);

        *logger << Logger::LogLevel::INFORMATIONAL << "info.json created from: " << infile << Logger::LogAction::FLUSH;
    }
    //=========================================================================


    pair<string,string> SipiHttpServer::get_canonical_url(int tmp_w, int tmp_h, const string &host, const string &prefix, const string &identifier, SipiRegion &region, SipiSize &size, SipiRotation &rotation, SipiQualityFormat &quality_format)
    {
        static const int canonical_len = 127;

        auto logger = Logger::getLogger(shttps::loggername);

        char canonical_region[canonical_len + 1];
        char canonical_size[canonical_len + 1];

        int tmp_r_x, tmp_r_y, tmp_r_w, tmp_r_h, tmp_red;
        bool tmp_ro;

        if (region.getType() != SipiRegion::FULL) {
            region.crop_coords(tmp_w, tmp_h, tmp_r_x, tmp_r_y, tmp_r_w, tmp_r_h);
        }
        region.canonical(canonical_region, canonical_len);

        if (size.getType() != SipiSize::FULL) {
            size.get_size(tmp_w, tmp_h, tmp_r_w, tmp_r_h, tmp_red, tmp_ro);
        }
        size.canonical(canonical_size, canonical_len);

        float angle;
        bool mirror = rotation.get_rotation(angle);

        char canonical_rotation[canonical_len + 1];
        if (mirror || (angle != 0.0)) {
            if ((angle - floorf(angle)) < 1.0e-6) { // it's an integer
                if (mirror) {
                    (void) snprintf(canonical_rotation, canonical_len, "!%ld", lroundf(angle));
                }
                else {
                    (void) snprintf(canonical_rotation, canonical_len, "%ld", lroundf(angle));
                }
            }
            else {
                if (mirror) {
                    (void) snprintf(canonical_rotation, canonical_len, "!%1.1f", angle);
                }
                else {
                    (void) snprintf(canonical_rotation, canonical_len, "%1.1f", angle);
                }
            }
            *logger << Logger::LogLevel::DEBUG << "Rotation (canonical): " << canonical_rotation << Logger::LogAction::FLUSH;
        }
        else {
            (void) snprintf(canonical_rotation, canonical_len, "0");
        }

        const unsigned canonical_header_len = 511;
        char canonical_header[canonical_header_len + 1];
        char ext[5];
        switch (quality_format.format()) {
            case SipiQualityFormat::JPG: {
                ext[0] = 'j'; ext[1] = 'p'; ext[2] = 'g'; ext[3] = '\0'; break; // jpg
            }
            case SipiQualityFormat::JP2: {
                ext[0] = 'j'; ext[1] = 'p'; ext[2] = '2'; ext[3] = '\0'; break; // jp2
            }
            case SipiQualityFormat::TIF: {
                ext[0] = 't'; ext[1] = 'i'; ext[2] = 'f'; ext[3] = '\0'; break; // tif
            }
            case SipiQualityFormat::PNG: {
                ext[0] = 'p'; ext[1] = 'n'; ext[2] = 'g'; ext[3] = '\0'; break; // png
            }
            case SipiQualityFormat::PDF: {
                ext[0] = 'p'; ext[1] = 'd'; ext[2] = 'f'; ext[3] = '\0'; break; // pdf
            }
            default: {
                throw SipiError(__file__, __LINE__, "Unsupported file format requested! Supported are .jpg, .jp2, .tif, .png, .pdf");
            }
        }

        (void) snprintf(canonical_header, canonical_header_len, "<http://%s/%s/%s/%s/%s/%s/default.%s>; rel=\"canonical\"",
                        host.c_str(), prefix.c_str(), identifier.c_str(), canonical_region, canonical_size, canonical_rotation, ext);
        string canonical = host + "/" + prefix + "/" + identifier + "/" + string(canonical_region) + "/" +
                           string(canonical_size) + "/" + string(canonical_rotation) + "/default." + string(ext);

        return make_pair(canonical_header, canonical);
    }
    //=========================================================================


    static void process_get_request(Connection &conobj, shttps::LuaServer &luaserver, void *user_data, void *dummy)
    {
        auto logger = Logger::getLogger(shttps::loggername);
        SipiHttpServer *serv = (SipiHttpServer *) user_data;

        bool prefix_as_path = serv->prefix_as_path();

        string uri = conobj.uri();

        vector<string> params;
        size_t pos = 0;
        size_t old_pos = 0;
        while ((pos = uri.find('/', pos)) != string::npos) {
            pos++;
            if (pos == 1) { // if first char is a token skip it!
                old_pos = pos;
                continue;
            }
            params.push_back(uri.substr(old_pos, pos - old_pos - 1));
            old_pos = pos;
        }
        if (old_pos != uri.length()) {
            params.push_back(uri.substr(old_pos, string::npos));
        }
        //for (int i = 0; i < params.size(); i++) cerr << params[i] << endl;

        if (params.size() < 1) {
            send_error(conobj, Connection::BAD_REQUEST, "No parameters/path given!");
            return;
        }

        params.push_back(uri.substr(old_pos, string::npos));

        //
        // if we just get the base URL, we redirect to the image info document
        //
        if (params.size() == 3) {
            string infile;
            if (prefix_as_path) {
                infile = serv->imgroot() + "/" + urldecode(params[iiif_prefix]) + "/" + urldecode(params[iiif_identifier]);
            }
            else {
                infile = serv->imgroot() + "/" + urldecode(params[iiif_identifier]);
            }

            if (access(infile.c_str(), R_OK) == 0) {
                conobj.setBuffer();
                conobj.status(Connection::SEE_OTHER);
                const string host = conobj.header("host");
                string redirect = string("http://") + host + "/" + params[iiif_prefix] + "/" + params[iiif_identifier] + "/info.json";
                conobj.header("Location", redirect);
                conobj.header("Content-Type", "text/plain");
                conobj << "Redirect to " << redirect;
                *logger << Logger::LogLevel::INFORMATIONAL << "GET: redirect to \"" << redirect << "\"." << Logger::LogAction::FLUSH;
                conobj.flush();
                return;
            }
            else {
                *logger << Logger::LogLevel::WARNING << "GET: \"" << infile << "\" not accessible!" << Logger::LogAction::FLUSH;
                send_error(conobj, Connection::NOT_FOUND);
                conobj.flush();
                return;
            }
        }

        //
        // test if there are enough parameters to fullfill the info request
        //
        if (params.size() < 3) {
            send_error(conobj, Connection::BAD_REQUEST, "Query syntax has not enough parameters!");
            return;
        }

        //string prefix = urldecode(params[iiif_prefix]);
        //string identifier = urldecode(params[iiif_identifier]);

        //
        // we have a request for the info json
        //
        if (params[iiif_region] == "info.json") {
            iiif_send_info(conobj, serv, luaserver, params, serv->imgroot(), prefix_as_path);
            return;
        }


        if (params.size() < 7) {
            send_error(conobj, Connection::BAD_REQUEST, "Query syntax has not enough parameters!");
            return;
        }
        if (params.size() > 7) {
            send_error(conobj, Connection::NOT_FOUND, "Too many \"/\"'s – imageid not found!");
            return;
        }

         //
        // getting region parameters
        //
        SipiRegion region;
        try {
            region = SipiRegion(params[iiif_region]);
            stringstream ss;
            ss << region;
            *logger << Logger::LogLevel::DEBUG << ss.str() << Logger::LogAction::FLUSH;
        }
        catch (Sipi::SipiError &err) {
            send_error(conobj, Connection::BAD_REQUEST, err);
            return;
        }

        //
        // getting scaling/size parameters
        //
        SipiSize size;
        try {
            size = SipiSize(params[iiif_size]);
            stringstream ss;
            ss << size;
            *logger << Logger::LogLevel::DEBUG << ss.str() << Logger::LogAction::FLUSH;
        }
        catch (Sipi::SipiError &err) {
            send_error(conobj, Connection::BAD_REQUEST, err);
            return;
        }

        //
        // getting rotation parameters
        //
        SipiRotation rotation;
        try {
            rotation = SipiRotation(params[iiif_rotation]);
            stringstream ss;
            ss << rotation;
            *logger << Logger::LogLevel::DEBUG << ss.str() << Logger::LogAction::FLUSH;
        }
        catch (Sipi::SipiError &err) {
            send_error(conobj, Connection::BAD_REQUEST, err);
            return;
        }

        SipiQualityFormat quality_format;
        try {
            quality_format = SipiQualityFormat(params[iiif_qualityformat]);
            stringstream ss;
            ss << quality_format;
            *logger << Logger::LogLevel::DEBUG << ss.str() << Logger::LogAction::FLUSH;
        }
        catch (Sipi::SipiError &err) {
            send_error(conobj, Connection::BAD_REQUEST, err);
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
                salsah = Salsah(&conobj, urldecode(params[iiif_identifier]));
            }
            catch (Sipi::SipiError &err) {
                send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }


            infile = salsah.getFilepath();
            if (salsah.getRights() < Salsah::RESOURCE_ACCESS_VIEW_RESTRICTED) {
                send_error(conobj, Connection::FORBIDDEN, "No right to view image!");
                return;
            }
        }
        else {
            infile = serv->imgroot() + "/" + prefix + "/" + identifier;
        }
        */



        string infile;  // path to the input file on the server
        string permission; // the permission string
        string watermark; // path to watermark file, or empty, if no watermark required
        SipiSize restriction_size; // size of restricted image... (SizeType::FULL if unrestricted)

        //
        // here we start the lua script which checks for permissions
        //
        const string funcname = "pre_flight";
        if (luaserver.luaFunctionExists(&funcname)) {
            LuaValstruct lval[3];
            lval[0].type = LuaValstruct::STRING_TYPE;
            lval[0].value.s = urldecode(params[iiif_prefix]);

            lval[1].type = LuaValstruct::STRING_TYPE;
            lval[1].value.s = urldecode(params[iiif_identifier]);

            string cookie = conobj.header("cookie");
            lval[2].type = LuaValstruct::STRING_TYPE;
            lval[2].value.s = cookie;

            vector<LuaValstruct> rval;
            try {
                rval = luaserver.executeLuafunction(&funcname, 3, lval);
            }
            catch (shttps::Error err) {
                send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err.to_string());
                return;
            }

            if (rval[0].type == LuaValstruct::STRING_TYPE) {
                permission = rval[0].value.s;
            }
            else {
                 send_error(conobj, Connection::INTERNAL_SERVER_ERROR, "Lua function pre_flight must return two strings");
                 return;
            }

            if (rval[1].type == LuaValstruct::STRING_TYPE) {
                infile = rval[1].value.s;
            }
            else {
                send_error(conobj, Connection::INTERNAL_SERVER_ERROR, "Lua function pre_flight must return two strings");
                return;
           }

            size_t pos = permission.find(':');
            string qualifier;
            if (pos != string::npos) {
                qualifier = permission.substr(pos + 1);
                permission = permission.substr(0, pos);
            }
            if (permission != "allow") {
                if (permission == "restrict") {
                    pos = qualifier.find('=');
                    string restriction_type = qualifier.substr(0, pos);
                    string restriction_param = qualifier.substr(pos + 1);
                    if (restriction_type == "watermark") {
                        watermark = restriction_param;
                    }
                    else if (restriction_type == "size") {
                        restriction_size = SipiSize(restriction_param);
                    }
                    else {
                        send_error(conobj, Connection::UNAUTHORIZED, "Unauthorized access!");
                        return;
                    }
                }
                else {
                    send_error(conobj, Connection::UNAUTHORIZED, "Unauthorized access!");
                    return;
                }
            }
        }
        else {
            if (prefix_as_path) {
                infile = serv->imgroot() + "/" + urldecode(params[iiif_prefix]) + "/" +
                         urldecode(params[iiif_identifier]);
            }
            else {
                infile = serv->imgroot() + "/" + urldecode(params[iiif_identifier]);
            }
        }

        size_t extpos = infile.find_last_of('.');
        string extension;
        SipiQualityFormat::FormatType in_format;
        if (extpos != string::npos) {
            extension = infile.substr(extpos + 1);
        }
        if ((extension == "tif") || (extension == "TIF") || (extension == "tiff") || (extension == "TIFF")) {
            in_format = SipiQualityFormat::TIF;
        }
        else if ((extension == "jpg") || (extension == "JPG")) {
            in_format = SipiQualityFormat::JPG;
        }
        else if ((extension == "png") || (extension == "PNG")) {
            in_format = SipiQualityFormat::PNG;
        }
        else if ((extension == "j2k") || (extension == "J2K") || (extension == "jp2") || (extension == "JP2") ||
                 (extension == "jpx") || (extension == "JPX")) {
            in_format = SipiQualityFormat::JP2;
        }
        else if ((extension == "pdf") || (extension == "PDF")) {
            in_format = SipiQualityFormat::PDF;
        }
        if (access(infile.c_str(), R_OK) != 0) { // test, if file exists
            send_error(conobj, Connection::NOT_FOUND);
            return;
        }


        float angle;
        bool mirror = rotation.get_rotation(angle);

        //
        // get cache info
        //
        SipiCache *cache = serv->cache();

        int img_w = 0, img_h = 0;
        if (in_format == SipiQualityFormat::PDF) {
            if (size.getType() != SipiSize::FULL) {
                send_error(conobj, Connection::BAD_REQUEST, "PDF must have size qualifier of \"full\"!");
                return;
            }
            if (region.getType() != SipiRegion::FULL) {
                send_error(conobj, Connection::BAD_REQUEST, "PDF must have region qualifier of \"full\"!");
                return;
            }
            float rot;
            if (rotation.get_rotation(rot) || (rot != 0.0)) {
                send_error(conobj, Connection::BAD_REQUEST, "PDF must have rotation qualifier of \"0\"!");
                return;
            }
            if ((quality_format.quality() != SipiQualityFormat::DEFAULT) || (quality_format.format() != SipiQualityFormat::PDF)) {
                send_error(conobj, Connection::BAD_REQUEST, "PDF must have quality qualifier of \"default.pdf\"!");
                return;
            }
        }
        else {
            //
            // get image dimensions, needed for get_canonical...
            //
            if ((cache == NULL) || !cache->getSize(infile, img_w, img_h)) {
                Sipi::SipiImage tmpimg;
                try {
                    tmpimg.getDim(infile, img_w, img_h);
                }
                catch(SipiImageError &err) {
                    send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err.get_error());
                    return;
                }
            }

            int tmp_r_w, tmp_r_h, tmp_red;
            bool tmp_ro;
            size.get_size(img_w, img_h, tmp_r_w, tmp_r_h, tmp_red, tmp_ro);
            restriction_size.get_size(img_w, img_h, tmp_r_w, tmp_r_h, tmp_red, tmp_ro);

            if (size > restriction_size) {
                size = restriction_size;
            }
        }


        //.....................................................................
        // here we start building the canonical URL
        //
        pair<string, string> tmppair;
        try {
            tmppair = serv->get_canonical_url(img_w, img_h, conobj.host(), params[iiif_prefix], params[iiif_identifier],
                                        region, size, rotation, quality_format);
        }
        catch(Sipi::SipiError &err) {
            send_error(conobj, Connection::BAD_REQUEST, err);
            return;
        }

        string canonical_header = tmppair.first;
        string canonical = tmppair.second;

        //
        // now we check if we can send the file directly
        //
        if ((region.getType() == SipiRegion::FULL) &&
            (size.getType() == SipiSize::FULL) &&
            (angle == 0.0) &&
            (!mirror) && watermark.empty() &&
            (quality_format.format() == in_format) &&
            (quality_format.quality() == SipiQualityFormat::DEFAULT)
        ) {
            *logger << Logger::LogLevel::DEBUG <<"Sending unmodified file...." << Logger::LogAction::FLUSH;
            conobj.status(Connection::OK);
            conobj.header("Cache-Control", "must-revalidate, post-check=0, pre-check=0");
            conobj.header("Link", canonical_header);
            switch (quality_format.format()) {
                case SipiQualityFormat::TIF: {
                    conobj.header("Content-Type", "image/tiff"); // set the header (mimetype)
                    break;
                }
                case SipiQualityFormat::JPG: {
                    conobj.header("Content-Type", "image/jpeg"); // set the header (mimetype)
                    break;
                }
                case SipiQualityFormat::PNG: {
                    conobj.header("Content-Type", "image/png"); // set the header (mimetype)
                    break;
                }
                case SipiQualityFormat::JP2: {
                    conobj.header("Content-Type", "image/jp2"); // set the header (mimetype)
                    break;
                }
                case SipiQualityFormat::PDF: {
                    conobj.header("Content-Type", "application/pdf"); // set the header (mimetype)
                    break;
                }
                default: {
                }
            }
            try {
                *logger << Logger::LogLevel::INFORMATIONAL <<  "Sending file: \"" << infile << "\"" << Logger::LogAction::FLUSH;
                conobj.sendFile(infile);
            }
            catch(int err) {
                // -1 was thrown
                *logger << Logger::LogLevel::WARNING <<  "Browser unexpectedly closed connection" << Logger::LogAction::FLUSH;
                return;
            }
            catch(Sipi::SipiError &err) {
                send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }
            return;
        }

        if (quality_format.format() == SipiQualityFormat::PDF) {
            send_error(conobj, Connection::BAD_REQUEST, "Conversion to PDF not yet supported!");
        }

        *logger << Logger::LogLevel::DEBUG << "Checking for cache..." << Logger::LogAction::FLUSH;

        if (cache != NULL) {
            *logger << Logger::LogLevel::DEBUG << "Cache found, testing for canonical '" << canonical << "'" << Logger::LogAction::FLUSH;
            string cachefile = cache->check(infile, canonical);
            if (!cachefile.empty()) {
                *logger << Logger::LogLevel::DEBUG << "Using cachefile '" << cachefile << "'" << Logger::LogAction::FLUSH;
                conobj.status(Connection::OK);
                conobj.header("Cache-Control", "must-revalidate, post-check=0, pre-check=0");
                conobj.header("Link", canonical_header);
                switch (quality_format.format()) {
                    case SipiQualityFormat::TIF: {
                        conobj.header("Content-Type", "image/tiff"); // set the header (mimetype)
                        break;
                    }
                    case SipiQualityFormat::JPG: {
                        conobj.header("Content-Type", "image/jpeg"); // set the header (mimetype)
                        break;
                    }
                    case SipiQualityFormat::PNG: {
                        conobj.header("Content-Type", "image/png"); // set the header (mimetype)
                        break;
                    }
                    case SipiQualityFormat::JP2: {
                        conobj.header("Content-Type", "image/jp2"); // set the header (mimetype)
                        break;
                    }
                    default: {
                    }
                }
                try {
                    *logger << Logger::LogLevel::DEBUG << "Sending cachefile '" << cachefile << "'" << Logger::LogAction::FLUSH;
                    conobj.sendFile(cachefile);
                }
                catch(int err) {
                    // -1 was thrown
                    *logger << Logger::LogLevel::WARNING << "Browser unexpectedly closed connection" << Logger::LogAction::FLUSH;
                    return;
                }
                catch(Sipi::SipiError &err) {
                    send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err);
                    return;
                }
                return;
            }
        }

        *logger << Logger::LogLevel::WARNING << "Nothing found in cache, reading and transforming file..." << Logger::LogAction::FLUSH;
        Sipi::SipiImage img;
        try {
            img.read(infile, &region, &size, quality_format.format() == SipiQualityFormat::JPG);
        }
        catch(const SipiImageError &err) {
            send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err.get_error());
            return;
        }

        //
        // now we rotate
        //
        if (mirror || (angle != 0.0)) {
            try {
                img.rotate(angle, mirror);
            }
            catch(Sipi::SipiError &err) {
                send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }
        }

        if (quality_format.quality() != SipiQualityFormat::DEFAULT) {
            switch (quality_format.quality()) {
                case SipiQualityFormat::COLOR: {
                    img.convertToIcc(icc_sRGB, 8); // for now, force 8 bit/sample
                }
                case SipiQualityFormat::GRAY: {
                    img.convertToIcc(icc_GRAY_D50, 8); // for now, force 8 bit/sample
                }
                default: {
                    // TODO: do nothing at the moment, bitonal is not yet supported...
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
            }
            catch(Sipi::SipiError &err) {
                send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }
            *logger << Logger::LogLevel::INFORMATIONAL << "GET: \"" << uri << "\": adding watermark" << Logger::LogAction::FLUSH;
        }


        img.connection(&conobj);
        conobj.header("Cache-Control", "must-revalidate, post-check=0, pre-check=0");

        string cachefile;
        if (cache != NULL) {
            cachefile = cache->getNewCacheName();
            *logger << Logger::LogLevel::INFORMATIONAL << "Writing new cache file '" << cachefile << "'" << Logger::LogAction::FLUSH;
        }
        try {
            switch (quality_format.format()) {
                case SipiQualityFormat::JPG: {
                    conobj.status(Connection::OK);
                    conobj.header("Link", canonical_header);
                    conobj.header("Content-Type", "image/jpeg"); // set the header (mimetype)
                    if ((img.getNc() > 3) && (img.getNalpha() > 0)) { // we have an alpha channel....
                        for (int i = 3; i < (img.getNalpha() + 3); i++) img.removeChan(i);
                    }
                    Sipi::SipiIcc icc = Sipi::SipiIcc(Sipi::icc_sRGB); // force sRGB !!
                    img.convertToIcc(icc, 8);
                    conobj.setChunkedTransfer();
                    if (cache != NULL) {
                        conobj.openCacheFile(cachefile);
                    }
                    *logger << Logger::LogLevel::DEBUG << "Before writing JPG..." << Logger::LogAction::FLUSH;
                    try {
                        img.write("jpg", "HTTP");
                    }
                    catch (SipiImageError &err) {
                        logger << err;
                        if (cache != NULL) {
                            conobj.closeCacheFile();
                            unlink(cachefile.c_str());
                        }
                        break;
                    }
                    *logger << Logger::LogLevel::DEBUG << "After writing JPG..." << Logger::LogAction::FLUSH;
                    if (cache != NULL) {
                        conobj.closeCacheFile();
                        *logger << Logger::LogLevel::DEBUG << "Adding cachefile '" << cachefile << "' to internal list" << Logger::LogAction::FLUSH;
                        cache->add(infile, canonical, cachefile, img_w, img_h);
                    }
                    break;
                }
                case SipiQualityFormat::JP2: {
                    conobj.status(Connection::OK);
                    conobj.header("Link", canonical_header);
                    conobj.header("Content-Type", "image/jp2"); // set the header (mimetype)
                    conobj.setChunkedTransfer();
                    *logger << Logger::LogLevel::DEBUG << "Before writing J2K..." << Logger::LogAction::FLUSH;
                    if (cache != NULL) {
                        conobj.openCacheFile(cachefile);
                    }
                    try {
                        img.write("jpx", "HTTP");
                    }
                    catch (SipiImageError &err) {
                        logger << err;
                        if (cache != NULL) {
                            conobj.closeCacheFile();
                            unlink(cachefile.c_str());
                        }
                    }
                    *logger << Logger::LogLevel::DEBUG << "After writing J2K..." << Logger::LogAction::FLUSH;
                    break;
                }
                case SipiQualityFormat::TIF: {
                    conobj.status(Connection::OK);
                    conobj.header("Link", canonical_header);
                    conobj.header("Content-Type", "image/tiff"); // set the header (mimetype)
                    // no chunked transfer needed...
                    *logger << Logger::LogLevel::DEBUG << "Before writing TIF..." << Logger::LogAction::FLUSH;
                    if (cache != NULL) {
                        conobj.openCacheFile(cachefile);
                    }
                    try {
                        img.write("tif", "HTTP");
                    }
                    catch (SipiImageError &err) {
                        logger << err;
                        if (cache != NULL) {
                            conobj.closeCacheFile();
                            unlink(cachefile.c_str());
                        }
                        break;
                    }
                    *logger << Logger::LogLevel::DEBUG << "After writing TIF..." << Logger::LogAction::FLUSH;
                    if (cache != NULL) {
                        conobj.closeCacheFile();
                        *logger << Logger::LogLevel::DEBUG << "Adding cachefile '" << cachefile << "' to internal list" << Logger::LogAction::FLUSH;
                        cache->add(infile, canonical, cachefile, img_w, img_h);
                    }
                    break;
                }
                case SipiQualityFormat::PNG: {
                    conobj.status(Connection::OK);
                    conobj.header("Link", canonical_header);
                    conobj.header("Content-Type", "image/png"); // set the header (mimetype)
                    conobj.setChunkedTransfer();
                    if (cache != NULL) {
                        conobj.openCacheFile(cachefile);
                    }
                    if (cache != NULL) {
                        conobj.openCacheFile(cachefile);
                    }
                    *logger << Logger::LogLevel::DEBUG << "Before writing PNG..." << Logger::LogAction::FLUSH;
                    try {
                        img.write("png", "HTTP");
                    }
                    catch (SipiImageError &err) {
                        logger << err;
                        if (cache != NULL) {
                            conobj.closeCacheFile();
                            unlink(cachefile.c_str());
                        }
                        break;
                    }
                    *logger << Logger::LogLevel::DEBUG << "After writing PNG..." << Logger::LogAction::FLUSH;
                    if (cache != NULL) {
                        conobj.closeCacheFile();
                        *logger << Logger::LogLevel::DEBUG << "Adding cachefile '" << cachefile << "' to internal list" << Logger::LogAction::FLUSH;
                        cache->add(infile, canonical, cachefile, img_w, img_h);
                    }
                    break;
                }
                default: {
                    // emitt HTTP CODE 400 !!! Format not supported!
                    *logger << Logger::LogLevel::WARNING << "Unsupported file format requested! Supported are .jpg, .jp2, .tif, .png" << Logger::LogAction::FLUSH;
                    conobj.setBuffer();
                    conobj.status(Connection::BAD_REQUEST);
                    conobj.header("Content-Type", "text/plain");
                    conobj << "Not Implemented!\n";
                    conobj <<"Unsupported file format requested! Supported are .jpg, .jp2, .tif, .png\n";
                    conobj.flush();
                }
            }
        }
        catch(Sipi::SipiError &err) {
            send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err);
            return;
        }

        conobj.flush();

        *logger << Logger::LogLevel::INFORMATIONAL << "GET: \"" << uri << "\": File: \"" << infile << "\"" << Logger::LogAction::FLUSH;
        return;
    }
    //=========================================================================


    static void favicon_handler(Connection &conobj, shttps::LuaServer &luaserver, void *user_data, void *dummy) {
        conobj.status(Connection::OK);
        conobj.header("Content-Type", "image/x-icon");
        conobj.send(favicon_ico, favicon_ico_len);
    }
    //=========================================================================

    static void test_handler(Connection &conobj, shttps::LuaServer &luaserver, void *user_data, void *dummy)
    {
        lua_State *L = luaL_newstate();
        luaL_openlibs(L);

        conobj.status(Connection::OK);
        conobj.header("Content-Type", "text/plain");
        conobj << "TEST test TEST test TEST!\n";

        lua_close(L);
    }
    //=========================================================================

    static void exit_handler(Connection &conobj, shttps::LuaServer &luaserver, void *user_data, void *dummy)
    {
        cerr << "exit handler called!" << endl;
        conobj.status(Connection::OK);
        conobj.header("Content-Type", "text/plain");
        conobj << "EXIT SIPI!\n";
        conobj.server()->stop();
    }
    //=========================================================================

    SipiHttpServer::SipiHttpServer(int port_p, unsigned nthreads_p, const std::string userid_str, const std::string &logfile_p, const std::string &loglevel_p)
        : Server::Server(port_p, nthreads_p, userid_str, logfile_p, loglevel_p)
    {
        _salsah_prefix = "imgrep";
        _cache = NULL;
    }
    //=========================================================================

    SipiHttpServer::~SipiHttpServer()
    {
        if (_cache != NULL) delete _cache;
    }
    //=========================================================================

    void SipiHttpServer::cache(const std::string &cachedir_p, long long max_cachesize_p, unsigned max_nfiles_p, float cache_hysteresis_p)
    {
        try {
            _cache = new SipiCache(cachedir_p, max_cachesize_p, max_nfiles_p, cache_hysteresis_p);
        }
        catch (const SipiError &err) {
            _cache = NULL;
            stringstream ss;
            ss << err;
            auto logger = Logger::getLogger(shttps::loggername);
            *logger << Logger::LogLevel::WARNING << "Couldn't open cache directory '" << cachedir_p << "'! Reason: " << ss.str() << Logger::LogAction::FLUSH;
            debugmsg("Warning: Couldn't open cache directory '" + cachedir_p + "'! Reason: " + ss.str());
        }
    }
    //=========================================================================

    void SipiHttpServer::run(void) {

        auto logger = Logger::getLogger(shttps::loggername);
        *logger << Logger::LogLevel::INFORMATIONAL << "SIPI server starting" << Logger::LogAction::FLUSH;

        //
        // setting the image root
        //
        *logger << Logger::LogLevel::INFORMATIONAL << "Serving images from \"" << _imgroot << "\"" << Logger::LogAction::FLUSH;
        *logger << Logger::LogLevel::INFORMATIONAL << "Salsah prefix \"" << _salsah_prefix << "\"" << Logger::LogAction::FLUSH;

        addRoute(Connection::GET, "/favcon.ico", favicon_handler);
        addRoute(Connection::GET, "/", process_get_request);
        addRoute(Connection::GET, "/admin/test", test_handler);
        addRoute(Connection::GET, "/admin/exit", exit_handler);

        user_data(this);

        Server::run();
    }
    //=========================================================================




}
