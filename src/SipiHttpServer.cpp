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
#include "iiifparser/SipiRegion.h"
#include "iiifparser/SipiRotation.h"
#include "iiifparser/SipiQualityFormat.h"
#include "PhpSession.h"
// #include "Salsah.h"

#include "Global.h"
#include "SipiHttpServer.h"
#include "Connection.h"

#include "shttps/cJSON.h"
#include "favicon.h"

#include "spdlog/spdlog.h"  // logging...

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
        auto logger = spdlog::get(shttps::loggername);
        conobj.status(code);
        conobj.setBuffer();
        conobj.header("Content-Type", "text/plain");
        switch (code) {
            case Connection::BAD_REQUEST:
                conobj << "Bad Request!";
                logger->error("Bad Request!") << errmsg;
                break;
            case Connection::FORBIDDEN:
                conobj << "Forbidden!";
                logger->error("Forbidden!") << errmsg;
                break;
            case Connection::NOT_FOUND:
                conobj << "Not Found!";
                logger->error("Not Found!") << errmsg;
                break;
            case Connection::INTERNAL_SERVER_ERROR:
                conobj << "Internal Server Error!";
                logger->error("Internal Server Error!") << errmsg;
                break;
            case Connection::NOT_IMPLEMENTED:
                conobj << "Not Implemented!";
                logger->error("Not Implemented!") << errmsg;
                break;
            case Connection::SERVICE_UNAVAILABLE:
                conobj << "Service Unavailable!";
                logger->error("Service Unavailable!") << errmsg;
                break;
            default:
                break; // do nothing
        }
        conobj << errmsg;
        logger->debug() << "GET: " << conobj.uri() << " failed!";
        conobj.flush();
    }
    //=========================================================================


    static void send_error(Connection &conobj, Connection::StatusCodes code, const SipiError &err) {
        auto logger = spdlog::get(shttps::loggername);
        conobj.status(code);
        conobj.setBuffer();
        conobj.header("Content-Type", "text/plain");
        stringstream outss;
        outss << err;
        switch (code) {
            case Connection::BAD_REQUEST:
                conobj << "Bad Request!";
                logger->error("Bad Request!") << outss.str();
                break;
            case Connection::FORBIDDEN:
                conobj << "Forbidden!";
                logger->error("Forbidden!") << outss.str();
                break;
            case Connection::NOT_FOUND:
                conobj << "Not Found!";
                logger->error("Not Found!") << outss.str();
                break;
            case Connection::INTERNAL_SERVER_ERROR:
                conobj << "Internal Server Error!";
                logger->error("Internal Server Error!") << outss.str();
                break;
            case Connection::NOT_IMPLEMENTED:
                conobj << "Not Implemented!";
                logger->error("Not Implemented!") << outss.str();
                break;
            case Connection::SERVICE_UNAVAILABLE:
                conobj << "Service Unavailable!";
                logger->error("Service Unavailable!") << outss.str();
                break;
            default: break; // do nothing
        }
        conobj << outss.str();
        logger->debug() << "GET: " << conobj.uri() << " failed!";
        conobj.flush();
    }
    //=========================================================================


    static void send_error(Connection &conobj, Connection::StatusCodes code) {
        auto logger = spdlog::get(shttps::loggername);
        conobj.status(code);
        conobj.setBuffer();
        conobj.header("Content-Type", "text/plain");
        switch (code) {
            case Connection::BAD_REQUEST:
                conobj << "Bad Request!";
                logger->error("Bad Request!");
                break;
            case Connection::NOT_FOUND:
                conobj << "Not Found!";
                logger->error("Not Found!");
                break;
            case Connection::INTERNAL_SERVER_ERROR:
                conobj << "Internal Server Error!";
                logger->error("Internal Server Error!");
                break;
            case Connection::NOT_IMPLEMENTED:
                conobj << "Not Implemented!";
                logger->error("Not Implemented!");
                break;
            case Connection::SERVICE_UNAVAILABLE:
                conobj << "Service Unavailable!";
                logger->error("Service Unavailable!");
                break;
            default: break; // do nothing
        }
        logger->debug() << "GET: " << conobj.uri() << " failed!";
        conobj.flush();
    }
    //=========================================================================


    static void iiif_send_info(Connection &conobj, SipiHttpServer *serv, shttps::LuaServer &luaserver, vector<string> &params, const string &imgroot, bool prefix_as_path) {
        auto logger = spdlog::get(shttps::loggername);
        conobj.setBuffer(); // we want buffered output, since we send JSON text...
        const string contenttype = conobj.header("content-type");

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
            rval = luaserver.executeLuafunction(&funcname, 3, lval);

            if (rval[0].type == LuaValstruct::STRING_TYPE) {
                permission = rval[0].value.s;
            }
            else { ; // error handling!
            }
            cerr << "Permission=" << permission << endl;

            if (rval[1].type == LuaValstruct::STRING_TYPE) {
                infile = rval[1].value.s;
            }
            else { ; // error handling!
            }

            size_t pos = permission.find('=');
            string qualifier;
            if (pos != string::npos) {
                qualifier = permission.substr(pos + 1);
                permission = permission.substr(0, pos);
            }
            if (permission != "view") {
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
            conobj.header("Content-Type", "application/ld-json");
        }
        else {
            conobj.header("Content-Type", "application/json");
            conobj.header("Link", "<http://iiif.io/api/image/2/context.json>; rel=\"http://www.w3.org/ns/json-ld#context\"; type=\"application/ld+json\"");
        }
        cJSON *root = cJSON_CreateObject();

        cJSON_AddItemToObject(root, "@context", cJSON_CreateString("http://iiif.io/api/image/2/context.json"));

        std::string host = conobj.header("host");
        std::string id = std::string("http://") + host + "/" + params[iiif_prefix] + "/" + params[iiif_identifier]; //// ?????????????????????????????????????
        cJSON_AddItemToObject(root, "@id", cJSON_CreateString(id.c_str()));

        cJSON_AddItemToObject(root, "protocol", cJSON_CreateString("http://iiif.io/api/image"));

        Sipi::SipiImage img;
        int width, height;
        img.getDim(infile, width, height);
        cJSON_AddItemToObject(root, "width", cJSON_CreateNumber(width));
        cJSON_AddItemToObject(root, "height", cJSON_CreateNumber(height));

        cJSON *sizes = cJSON_CreateArray();
        for (int i = 1; i < 5; i++) {
            SipiSize size(i);
            int w, h, r;
            bool ro;
            size.get_size(width, height, w, h, r, ro);
            if ((w < 128) && (h < 128)) break;
            cJSON *sobj = cJSON_CreateObject();
            cJSON_AddItemToObject(sobj, "width", cJSON_CreateNumber(w));
            cJSON_AddItemToObject(sobj, "height", cJSON_CreateNumber(h));
            cJSON_AddItemToArray(sizes, sobj);
        }
        cJSON_AddItemToObject(root, "sizes", sizes);

        cJSON *profile_arr = cJSON_CreateArray();
        cJSON_AddItemToArray(profile_arr, cJSON_CreateString("http://iiif.io/api/image/2/level2.json"));
        cJSON *profile = cJSON_CreateObject();

        const char * formats_str[] = {"tif", "jpg", "png", "jp2"};
        cJSON *formats = cJSON_CreateStringArray(formats_str, sizeof(formats_str)/sizeof(char*));
        cJSON_AddItemToObject(profile, "formats", formats);

        const char * qualities_str[] = {"color", "gray"};
        cJSON *qualities = cJSON_CreateStringArray(qualities_str, sizeof(qualities_str)/sizeof(char*));
        cJSON_AddItemToObject(profile, "qualities", qualities);

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
        cJSON *supports = cJSON_CreateStringArray(supports_str, sizeof(supports_str)/sizeof(char*));
        cJSON_AddItemToObject(profile, "supports", supports);

        cJSON_AddItemToArray(profile_arr, profile);

        cJSON_AddItemToObject(root, "profile", profile_arr);

        char *json_str = cJSON_Print(root);

        conobj.sendAndFlush(json_str, strlen(json_str));

        free (json_str);

        //TODO and all the other CJSON obj?
        cJSON_Delete(root);

        logger->info("info.json created from: ") << infile;
    }
    //=========================================================================


    pair<string,string> SipiHttpServer::get_canonical_url(int tmp_w, int tmp_h, const string &host, const string &prefix, const string &identifier, SipiRegion &region, SipiSize &size, SipiRotation &rotation, SipiQualityFormat &quality_format)
    {
        static const int canonical_len = 127;

        auto logger = spdlog::get(shttps::loggername);

        char canonical_region[canonical_len + 1];
        char canonical_size[canonical_len + 1];

        int tmp_r_x, tmp_r_y, tmp_r_w, tmp_r_h, tmp_red;
        bool tmp_ro;

        region.crop_coords(tmp_w, tmp_h, tmp_r_x, tmp_r_y, tmp_r_w, tmp_r_h);
        region.canonical(canonical_region, canonical_len);

        size.get_size(tmp_w, tmp_h, tmp_r_w, tmp_r_h, tmp_red, tmp_ro);
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
            logger->debug() << "Rotation (canonical): " << canonical_rotation;
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
            default: {
                throw SipiError(__file__, __LINE__, "Unsupported file format requested! Supported are .jpg, .jp2, .tif, .png");
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
        auto logger = spdlog::get(shttps::loggername);
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
            logger->error() << "GET failed: No parameters/path given!";
            send_error(conobj, Connection::BAD_REQUEST, "No parameters/path given!");
            return;
        }

        params.push_back(uri.substr(old_pos, string::npos));

        //
        // if we just get the base URL, we redirect to the image info document
        //
        if (params.size() == 2) {
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
                logger->info() << "GET: redirect to \"" << redirect << "\".";
                conobj.flush();
                return;
            }
            else {
                logger->warn() << "GET: \"" << infile << "\" not accessible!";
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
            logger->error() << "Query syntax has not enough parameters!";
            send_error(conobj, Connection::BAD_REQUEST, "Query syntax has not enough parameters!");
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
            logger->debug() << ss.str();
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
            logger->debug() << ss.str();
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
            logger->debug() << ss.str();
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
            logger->debug() << ss.str();
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
        SipiSize restriction_size; // size of restricted image... (SizeTYpe::FULL if unrestricted)
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
            rval = luaserver.executeLuafunction(&funcname, 3, lval);

            if (rval[0].type == LuaValstruct::STRING_TYPE) {
                permission = rval[0].value.s;
            }
            else { ; // error handling!
            }

            if (rval[1].type == LuaValstruct::STRING_TYPE) {
                infile = rval[1].value.s;
            }
            else { ; // error handling!
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
        if ((extension == "tif") || (extension == "TIF")) {
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
        if (access(infile.c_str(), R_OK) != 0) { // test, if file exists
            send_error(conobj, Connection::NOT_FOUND);
            return;
        }

        float angle;
        bool mirror = rotation.get_rotation(angle);

        //
        // get image dimensions, needed for get_canonical...
        //
        int img_w, img_h;
        Sipi::SipiImage tmpimg;
        tmpimg.getDim(infile, img_w, img_h);

        int tmp_r_w, tmp_r_h, tmp_red;
        bool tmp_ro;
        size.get_size(img_w, img_h, tmp_r_w, tmp_r_h, tmp_red, tmp_ro);
        restriction_size.get_size(img_w, img_h, tmp_r_w, tmp_r_h, tmp_red, tmp_ro);

        if (size > restriction_size) {
            size = restriction_size;
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
            (quality_format.format() == in_format)
        ) {
            logger->debug("Sending unmodified file....");
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
                logger->info() << "Sending file: \"" << infile << "\"";
                conobj.sendFile(infile);
            }
            catch(int err) {
                // -1 was thrown
                logger->error("Browser unexpectedly closed connection");
                cerr << "Browser unexpectedly closed connection" << endl;
                return;
            }
            catch(Sipi::SipiError &err) {
                send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }
            return;
        }

        logger->debug("Checking for cache...");

        SipiCache *cache = serv->cache();
        if (cache != NULL) {
            logger->debug("Cache found, testing for canonical '") << canonical << "'";
            string cachefile = cache->check(infile, canonical);
            if (!cachefile.empty()) {
                logger->debug("Using cachefile '") << cachefile << "'";
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
                    logger->info() << "Sending file: \"" << cachefile << "\"";
                    conobj.sendFile(cachefile);
                }
                catch(int err) {
                    // -1 was thrown
                    logger->error("Browser unexpectedly closed connection");
                    cerr << "Browser unexpectedly closed connection" << endl;
                    return;
                }
                catch(Sipi::SipiError &err) {
                    send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err);
                    return;
                }
                return;
            }
        }

        logger->debug("Nothing found in cache, reading and transforming file...");
        Sipi::SipiImage img;
        try {
            img.read(infile, &region, &size, quality_format.format() == SipiQualityFormat::JPG);
        }
        catch(const Sipi::SipiError &err) {
            send_error(conobj, Connection::INTERNAL_SERVER_ERROR, err);
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
            logger->info() << "GET: \"" << uri << "\": adding watermark";
        }


        img.connection(&conobj);
        conobj.header("Cache-Control", "must-revalidate, post-check=0, pre-check=0");

        string cachefile;
        if (cache != NULL) {
            cachefile = cache->getNewCacheName();
            logger->debug("Writing new cache file '") << cachefile << "'";
        }
        try {
            switch (quality_format.format()) {
                case SipiQualityFormat::JPG: {
                    conobj.status(Connection::OK);
                    conobj.header("Link", canonical_header);
                    conobj.header("Content-Type", "image/jpeg"); // set the header (mimetype)
                    Sipi::SipiIcc icc = Sipi::SipiIcc(Sipi::icc_sRGB);
                    img.convertToIcc(icc, 8);
                    conobj.setChunkedTransfer();
                    if (cache != NULL) {
                        conobj.openCacheFile(cachefile);
                    }
                    logger->debug("Before writing JPG...");
                    try {
                        img.write("jpg", "HTTP");
                    }
                    catch (SipiImageError &err) {
                        if (cache != NULL) {
                            conobj.closeCacheFile();
                            unlink(cachefile.c_str());
                        }
                        break;
                    }
                    logger->debug("After writing JPG...");
                    if (cache != NULL) {
                        conobj.closeCacheFile();
                        logger->debug("Adding cachefile '") << cachefile << "' to internal list";
                        cache->add(infile, canonical, cachefile);
                        logger->debug("DONE");
                    }
                    break;
                }
                case SipiQualityFormat::JP2: {
                    conobj.status(Connection::OK);
                    conobj.header("Link", canonical_header);
                    conobj.header("Content-Type", "image/jp2"); // set the header (mimetype)
                    conobj.setChunkedTransfer();
                    img.write("jpx", "HTTP");
                    break;
                }
                case SipiQualityFormat::TIF: {
                    conobj.status(Connection::OK);
                    conobj.header("Link", canonical_header);
                    conobj.header("Content-Type", "image/tiff"); // set the header (mimetype)
                    // no chunked transfer needed...
                    img.write("tif", "HTTP");
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
                    logger->debug("Before writing PNG...");
                    img.write("png", "HTTP");
                    logger->debug("After writing PNG...");
                    if (cache != NULL) {
                        conobj.closeCacheFile();
                        logger->debug("Adding cachefile '") << cachefile << "' to internal list";
                        cache->add(infile, canonical, cachefile);
                        logger->debug("DONE");
                    }
                    break;
                }
                default: {
                    // emitt HTTP CODE 400 !!! Format not supported!
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

        logger->info() << "GET: \"" << uri << "\": File: " << infile;
        return;
    }
    //=========================================================================


    static void favicon_handler(Connection &conobj, shttps::LuaServer &luaserver, void *user_data, void *dummy) {
        conobj.status(Connection::OK);
        conobj.header("Content-Type", "image/x-icon");
        conobj.send(favicon_ico, favicon_ico_len);
    }
    //=========================================================================

    static void admin_handler(Connection &conobj, shttps::LuaServer &luaserver, void *user_data, void *dummy)
    {
        conobj.status(Connection::OK);
        conobj.header("Content-Type", "text/plain");
        //conobj.add_header("Content-Length", "28");
        conobj << "(1) Hello World, I'm here again!\n";
        sleep(2);
        conobj << "(2) Hello World, I'm here again!\n";
        conobj.flush();
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

    SipiHttpServer::SipiHttpServer(int port_p, unsigned nthreads_p, const std::string &logfile_p)
        : Server::Server(port_p, nthreads_p, logfile_p)
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
            _logger->error("Couldn't open cache directory '") << cachedir_p << "'!";
            stringstream ss;
            ss << err;
            _logger->error(ss.str());
        }
    }
    //=========================================================================

    void SipiHttpServer::run(void) {

        _logger->info() << "SIPI server starting";

        //
        // setting the image root
        //
        _logger->info() << "Serving images from \"" << _imgroot << "\"";
        _logger->info() << "Salsah prefix \"" << _salsah_prefix << "\"";

        addRoute(Connection::GET, "/admin", admin_handler);
        addRoute(Connection::GET, "/favcon.ico", favicon_handler);
        addRoute(Connection::GET, "/", process_get_request);
        addRoute(Connection::GET, "/admin/test", test_handler);
        addRoute(Connection::GET, "/admin/exit", exit_handler);

        user_data(this);

        Server::run();
    }
    //=========================================================================




}
