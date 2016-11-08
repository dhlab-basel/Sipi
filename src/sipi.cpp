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
 * \brief Implements a simple HTTP server.
 *
 */
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <csignal>
#include <utility>

#include "shttps/Global.h"
#include "shttps/LuaServer.h"
#include "shttps/LuaSqlite.h"
#include "SipiLua.h"
#include "SipiCmdParams.h"
#include "SipiImage.h"
#include "SipiHttpServer.h"

#include "jansson.h"
#include "shttps/GetMimetype.h"
#include "SipiConf.h"

/*!
 * \mainpage
 *
 * # SIPI – Simple Image Presentation Interface #
 *
 * SIPI is a package that can be used to convert images from/to different formats while
 * preserving as much metadata thats embeded in the file headers a possible. SIPI is also
 * able to do some conversions, especially some common color space transformation using
 * ICC profiles. Currently SIPI supports the following file formats
 *
 * - TIFF
 * - JPEG2000
 * - PNG
 * - JPEG
 *
 * The following metadata "standards" are beeing preserved
 * - EXIF
 * - IPTC
 * - XMP
 *
 * ## Commandline Use ##
 *
 * For simple conversions, SIPI is being used from the command line (in a terminal window). The
 * format is usually
 *
 *     sipi [options] <infile> <outfile>
 *
 */
Sipi::SipiHttpServer *serverptr = NULL;
Sipi::SipiConf sipiConf;
enum FileType {image, video, audio, text, binary};

static sig_t old_sighandler;
static sig_t old_broken_pipe_handler;

static std::string fileType_string(FileType f_type) {
    std::string type_string;

    switch (f_type) {
        case image:
            type_string = "image";
            break;
        case video:
            type_string = "video";
            break;
        case audio:
            type_string = "audio";
            break;
        case text:
            type_string = "text";
            break;
        case binary:
            type_string = "binary";
            break;
    }

    return type_string;
};

static void sighandler(int sig) {
    std::cerr << std::endl << "Got SIGINT, stopping server gracefully...." << std::endl;
    if (serverptr != NULL) {
        auto logger = spdlog::get(shttps::loggername);
        logger->info("Got SIGINT, stopping server");
        serverptr->stop();
    }
    else {
        auto logger = spdlog::get(shttps::loggername);
        logger->info("Got SIGINT, exiting server");
        exit(0);
    }
}
//=========================================================================


static void broken_pipe_handler(int sig) {
    auto logger = spdlog::get(shttps::loggername);
    logger->info("Got BROKEN PIPE signal!");
}
//=========================================================================




static void send_error(shttps::Connection &conobj, shttps::Connection::StatusCodes code, std::string msg)
{
    conobj.status(code);
    conobj.header("Content-Type", "application/json");

    json_t *root = json_object();
    json_object_set_new(root, "status", json_integer(1));
    json_object_set_new(root, "message", json_string(msg.c_str()));

    char *json_str = json_dumps(root, JSON_INDENT(3));
    conobj << json_str << shttps::Connection::flush_data;
    free (json_str);
    json_decref(root);
}
//=========================================================================


static void send_error(shttps::Connection &conobj, shttps::Connection::StatusCodes code, Sipi::SipiError &err)
{
    conobj.status(code);
    conobj.header("Content-Type", "application/json");

    json_t *root = json_object();
    json_object_set_new(root, "status", json_integer(1));
    std::stringstream ss;
    ss << err;
    json_object_set_new(root, "message", json_string(ss.str().c_str()));
    char *json_str = json_dumps(root, JSON_INDENT(3));
    conobj << json_str << shttps::Connection::flush_data;
    free (json_str);
    json_decref(root);
}
//=========================================================================

static void sipiConfGlobals(lua_State *L, shttps::Connection &conn, void *user_data) {
    Sipi::SipiConf *conf = (Sipi::SipiConf *) user_data;

    lua_createtable(L, 0, 14); // table1

    lua_pushstring(L, "port"); // table1 - "index_L1"
    lua_pushinteger(L, conf->getPort());
    lua_rawset(L, -3); // table1

#ifdef SHTTPS_ENABLE_SSL

    lua_pushstring(L, "sslport"); // table1 - "index_L1"
    lua_pushinteger(L, conf->getSSLPort());
    lua_rawset(L, -3); // table1

#endif

    lua_pushstring(L, "imgroot"); // table1 - "index_L1"
    lua_pushstring(L, conf->getImgRoot().c_str());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "prefix_as_path"); // table1 - "index_L1"
    lua_pushboolean(L, conf->getPrefixAsPath());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "init_script"); // table1 - "index_L1"
    lua_pushstring(L, conf->getInitScript().c_str());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "cache_dir"); // table1 - "index_L1"
    lua_pushstring(L, conf->getCacheDir().c_str());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "cache_size"); // table1 - "index_L1"
    lua_pushstring(L, conf->getCacheSize().c_str());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "cache_hysteresis"); // table1 - "index_L1"
    lua_pushnumber(L, conf->getCacheHysteresis());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "keep_alive"); // table1 - "index_L1"
    lua_pushinteger(L, conf->getKeepAlive());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "thumb_size"); // table1 - "index_L1"
    lua_pushstring(L, conf->getThumbSize().c_str());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "cache_n_files"); // table1 - "index_L1"
    lua_pushinteger(L, conf->getCacheNFiles());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "n_threads"); // table1 - "index_L1"
    lua_pushinteger(L, conf->getNThreads());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "tmpdir"); // table1 - "index_L1"
    lua_pushstring(L, conf->getTmpDir().c_str());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "scriptdir"); // table1 - "index_L1"
    lua_pushstring(L, conf->getScriptDir().c_str());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "knora_path"); // table1 - "index_L1"
    lua_pushstring(L, conf->getKnoraPath().c_str());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "knora_port"); // table1 - "index_L1"
    lua_pushstring(L, conf->getKnoraPort().c_str());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "adminuser"); // table1 - "index_L1"
    lua_pushstring(L, conf->getAdminUser().c_str());
    lua_rawset(L, -3); // table1

    lua_pushstring(L, "password"); // table1 - "index_L1"
    lua_pushstring(L, conf->getPassword().c_str());
    lua_rawset(L, -3); // table1

    lua_setglobal(L, "config");
}


int main (int argc, char *argv[]) {

    //
    // register namespace sipi in xmp. Since this part of the XMP library is
    // not reentrant, it must be done here in the main thread!
    //
    if (!Exiv2::XmpParser::initialize(Sipi::xmplock_func, &Sipi::xmp_mutex)) {
        std::cerr << "Exiv2::XmpParser::initialize failed" << std::endl;
    }


    //
    // commandline processing....
    //
    Sipi::SipiCmdParams params (argc, argv, "A generic image format converter preserving the metadata");
    params.addParam(new Sipi::SipiParam("format", "Output format", "jpx:jpg:tif:png", 1, "jpx"));
    params.addParam(new Sipi::SipiParam("icc", "Convert to ICC profile", "none:sRGB:AdobeRGB:GRAY", 1, "none"));
    params.addParam(new Sipi::SipiParam("quality", "Quality (compression)", 1, 100, 1, 80));
    params.addParam(new Sipi::SipiParam("region", "Select region of interest (x,y,w,h)", -1, 9999, 4, 0, 0, -1, -1));
    params.addParam(new Sipi::SipiParam("reduce", "Reduce image size by factor (Cannot be used together with \"-size\" and \"-scale\")", 0, 5, 1, 0));
    params.addParam(new Sipi::SipiParam("size", "Resize image to given size (Cannot be used together with \"-reduce\" and \"-scale\")", 0, 999999, 2, 0, 0));
    params.addParam(new Sipi::SipiParam("scale", "Resize image by the given percentage (Cannot be used together with \"-size\" and \"-reduce\")", 0.0F, 1000.F, 1, 100.F));
    params.addParam(new Sipi::SipiParam("skipmeta", "Skip the given metadata", "none:all", 1, "none"));
    params.addParam(new Sipi::SipiParam("mirror", "Mirror the image", "none:horizontal:vertical", 1, "none"));
    params.addParam(new Sipi::SipiParam("rotate", "Rotate the image", 0.0F, 359.99999F, 1, 0.0F));
    params.addParam(new Sipi::SipiParam("salsah", "Special flag for SALSAH internal use", false));
    params.addParam(new Sipi::SipiParam("compare", "compare to files", false));
    params.addParam(new Sipi::SipiParam("serverport", "Port of the webserver", 0, 65535, 1, 0));
    params.addParam(new Sipi::SipiParam("nthreads", "Number of threads for webserver", -1, 64, 1, -1));
    params.addParam(new Sipi::SipiParam("imgroot", "Root directory containing the images (webserver)", 1, "."));
    params.addParam(new Sipi::SipiParam("config", "Configuration file for webserver", 1, ""));
    params.addParam(new Sipi::SipiParam("loglevel", "Logging level", "TRACE:DEBUG:INFO:NOTICE:WARN:ERROR:CRITICAL:ALERT:EMER:OFF", 1, "INFO"));
    params.parseArgv ();


    if (params["compare"].isSet()) {
        std::string infname1;
        try {
            infname1 = params.getName();
        }
        catch (Sipi::SipiError &err) {
            std::cerr << err;
            exit (-1);
        }

        //
        // get the output image name
        //
        std::string infname2;
        try {
            infname2 = params.getName();
        }
        catch (Sipi::SipiError &err) {
            std::cerr << err;
            exit (-1);
        }
        Sipi::SipiImage img1, img2;
        img1.read(infname1);
        img2.read(infname2);
        bool result = img1.compare(img2);

        return (result) ? 0 : -1;
    }


    //
    // if a config file is given, we start sipi as IIIF compatible server
    //
    if ((params["config"]).isSet()) {
        std::string configfile = (params["config"])[0].getValue(SipiStringType);
        try {
            std::cout << std::endl << SIPI_VERSION << std::endl;
            //read and parse the config file (config file is a lua script)
            shttps::LuaServer luacfg(configfile);

            //store the config option in a SipiConf obj
            sipiConf = Sipi::SipiConf(luacfg);

            //Create object SipiHttpServer
            Sipi::SipiHttpServer server(sipiConf.getPort(), sipiConf.getNThreads(),
                sipiConf.getUseridStr(), sipiConf.getLogfile(), sipiConf.getLoglevel());

#           ifdef SHTTPS_ENABLE_SSL

            server.ssl_port(sipiConf.getSSLPort()); // set the secure connection port (-1 means no ssl socket)
            std::string tmps = sipiConf.getSSLCertificate();
            server.ssl_certificate(tmps);
            tmps = sipiConf.getSSLKey();
            server.ssl_key(tmps);
            server.jwt_secret(sipiConf.getJwtSecret());

#           endif

            // set tmpdir for uploads (defined in sipi.config.lua)
            server.tmpdir(sipiConf.getTmpDir());

            server.scriptdir(sipiConf.getScriptDir()); // set the directory where the Lua scripts are found for the "Lua"-routes
            server.luaRoutes(sipiConf.getRoutes());
            server.add_lua_globals_func(sipiConfGlobals, &sipiConf);
            server.add_lua_globals_func(shttps::sqliteGlobals); // add new lua function "gaga"
            server.add_lua_globals_func(Sipi::sipiGlobals, &server); // add Lua SImage functions
            server.prefix_as_path(sipiConf.getPrefixAsPath());


            //
            // cache parameter...
            //
            std::string emptystr;
            std::string cachedir = sipiConf.getCacheDir();
            if (!cachedir.empty()) {
                std::string cachesize_str = sipiConf.getCacheSize();
                long long cachesize = 0;
                if (!cachesize_str.empty()) {
                    size_t l = cachesize_str.length();
                    char c = cachesize_str[l - 1];
                    if (c == 'M') {
                        cachesize = stoll(cachesize_str.substr(0, l - 1))*1024*1024;
                    }
                    else if (c == 'G') {
                        cachesize = stoll(cachesize_str.substr(0, l - 1))*1024*1024*1024;
                    }
                    else {
                        cachesize = stoll(cachesize_str);
                    }
                }
                int nfiles = sipiConf.getCacheNFiles();
                float hysteresis = sipiConf.getCacheHysteresis();
                server.cache(cachedir, cachesize, nfiles, hysteresis);
            }

            server.imgroot(sipiConf.getImgRoot());
            server.initscript(sipiConf.getInitScript());

            server.keep_alive_timeout(sipiConf.getKeepAlive());

            //
            // now we set the routes for the normal HTTP server file handling
            //
            std::string docroot = sipiConf.getDocRoot();
            std::string docroute = sipiConf.getDocRoute();
            std::pair<std::string,std::string> filehandler_info;

            if (!(docroute.empty() || docroot.empty())) {
                filehandler_info.first = docroute;
                filehandler_info.second = docroot;
                server.addRoute(shttps::Connection::GET, docroute, shttps::FileHandler, &filehandler_info);
                server.addRoute(shttps::Connection::POST, docroute, shttps::FileHandler, &filehandler_info);
            }

            serverptr = &server;
            old_sighandler = signal(SIGINT, sighandler);
            old_broken_pipe_handler = signal(SIGPIPE, broken_pipe_handler);
            server.run();
        }
        catch (shttps::Error &err) {
            std::cerr << err << std::endl;
        }
    }

    //
    // if a server port is given, we start sipi as IIIF compatible server on the given port
    //
    else if ((params["serverport"])[0].getValue (SipiIntType) > 0) {
        int nthreads = (params["nthreads"])[0].getValue (SipiIntType);
        if (nthreads == -1) nthreads = std::thread::hardware_concurrency();
        Sipi::SipiHttpServer server((params["serverport"])[0].getValue (SipiIntType), nthreads);
        server.imgroot((params["imgroot"])[0].getValue(SipiStringType));
        serverptr = &server;
        old_sighandler = signal(SIGINT, sighandler);
        old_broken_pipe_handler = signal(SIGPIPE, broken_pipe_handler);
        server.run();
    }
    else {
        //
        // get the input image name
        //
        std::string infname;
        try {
            infname = params.getName();
        }
        catch (Sipi::SipiError &err) {
            std::cerr << err;
            exit (-1);
        }

        //
        // get the output image name
        //
        std::string outfname;
        try {
            outfname = params.getName();
        }
        catch (Sipi::SipiError &err) {
            std::cerr << err;
            exit (-1);
        }

        //
        // get the output format
        //
        std::string format;
        try {
            format = (params["format"])[0].getValue(SipiStringType);
        }
        catch (Sipi::SipiError &err) {
            std::cerr << err;
            exit (-1);
        }


        //
        // getting information about a region of interest
        //
        Sipi::SipiRegion *region = NULL;
        if (params["region"].isSet()) {
            region = new Sipi::SipiRegion((params["region"])[0].getValue(SipiIntType),
            (params["region"])[1].getValue(SipiIntType),
            (params["region"])[2].getValue(SipiIntType),
            (params["region"])[3].getValue(SipiIntType));
        }

        Sipi::SipiSize *size = NULL;
        //
        // get the reduce parameter
        // "reduce" is a special feature of the JPEG2000 format. It is possible (given the JPEG2000 format
        // is written a resolution pyramid). reduce=0 results in full resolution, reduce=1 is half the resolution
        // etc.
        //
        int reduce = params["reduce"].isSet() ? (params["reduce"])[0].getValue(SipiIntType): 0;
        if (reduce > 0) {
            size = new Sipi::SipiSize(reduce);
        }
        else if (params["size"].isSet()) {
            size = new Sipi::SipiSize((params["size"])[0].getValue(SipiIntType), (params["size"])[1].getValue(SipiIntType));
        }
        else if (params["scale"].isSet()) {
            size = new Sipi::SipiSize((params["scale"])[0].getValue(SipiFloatType));
        }

        //
        // read the input image
        //
        Sipi::SipiImage img;
        img.read(infname, region, size, format == "jpg"); //convert to bps=8 in case of JPG output

        delete region;
        delete size;

        //
        // if we want to remove all metadata from the file...
        //
        std::string skipmeta = (params["skipmeta"])[0].getValue(SipiStringType);
        if (skipmeta != "none") {
            img.setSkipMetadata(Sipi::SKIP_ALL);
        }

        //
        // color profile processing
        //
        std::string iccprofile = (params["icc"])[0].getValue(SipiStringType);

        if (iccprofile != "none") {
            Sipi::SipiIcc icc;
            if (iccprofile == "sRGB") {
                icc = Sipi::SipiIcc(Sipi::icc_sRGB);
            }
            else if (iccprofile == "AdobeRGB") {
                icc = Sipi::SipiIcc(Sipi::icc_AdobeRGB);
            }
            else if (iccprofile == "GRAY") {
                icc = Sipi::SipiIcc(Sipi::icc_GRAY_D50);
            }
            else {
                icc = Sipi::SipiIcc(Sipi::icc_sRGB);
            }
            img.convertToIcc(icc, 8);
        }

        //
        // mirroring and rotation
        //
        std::string mirror = (params["mirror"])[0].getValue(SipiStringType);
        float angle = (params["rotate"])[0].getValue(SipiFloatType);

        if (mirror != "none") {
            if (mirror == "horizontal") {
                img.rotate(angle, true);
            }
            else if (mirror == "vertical") {
                angle += 180.0F;
                img.rotate(angle, true);
            }
            else {
                img.rotate(angle, false);
            }
        }
        else if (angle != 0.0F) {
            img.rotate(angle, false);
        }

        //std::cout << img << std::endl;

        /*
        std::cerr << ">>>> Sipi::SipiIcc rgb_icc = Sipi::SipiIcc(Sipi::icc_AdobeRGB):" << std::endl;
        Sipi::SipiIcc rgb_icc(Sipi::icc_AdobeRGB);

        std::cerr << ">>>> img.convertToIcc(rgb_icc, 8):" << std::endl;
        img.convertToIcc(rgb_icc, 8);

        std::cout << img << std::endl;
        std::cerr << ">>>> img.write(" << outfname << "):" << std::endl;
        */

        //
        // write the output file
        //
        try {
            img.write(format, outfname, (params["quality"])[0].getValue (SipiIntType));
        }
        catch (Sipi::SipiImageError &err) {
            std::cerr << err.what() << std::endl;
        }

        if (params["salsah"].isSet()) {
            std::cout << img.getNx() << " " << img.getNy() << std::endl;
        }

    }
    return 0;
}
