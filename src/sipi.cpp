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
#include <syslog.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <utility>
#include <stdlib.h>

#include "curl/curl.h"
#include "shttps/Global.h"
#include "shttps/LuaServer.h"
#include "shttps/LuaSqlite.h"
#include "SipiLua.h"
#include "SipiCmdParams.h"
#include "SipiImage.h"
#include "SipiHttpServer.h"
#include "optionparser.h"

#include "jansson.h"
#include "shttps/GetMimetype.h"
#include "SipiConf.h"

/*!
 * \mainpage
 *
 * # Sipi – Simple Image Presentation Interface #
 *
 * Sipi is a package that can be used to convert images from/to different formats while
 * preserving as much metadata thats embeded in the file headers a possible. Sipi is also
 * able to do some conversions, especially some common color space transformation using
 * ICC profiles. Currently Sipi supports the following file formats
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
 * For simple conversions, Sipi is being used from the command line (in a terminal window). The
 * format is usually
 *
 *     sipi [options] <infile> <outfile>
 *
 */
Sipi::SipiHttpServer *serverptr = NULL;
Sipi::SipiConf sipiConf;
enum FileType {image, video, audio, text, binary};

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

    // TODO: in the sipi config file, there are different namespaces that are unified here (danger of collision)
    lua_pushstring(L, "docroot"); // table1 - "index_L1"
    lua_pushstring(L, conf->getDocRoot().c_str());
    lua_rawset(L, -3); // table1

    lua_setglobal(L, "config");
}
enum  optionIndex { UNKNOWN, CONFIGFILE, FILEIN , FORMAT, ICC, QUALITY, REGION, REDUCE, SIZE, SCALE,
                    SKIPMETA, MIRROR, ROTATE, SALSAH, COMPARE, SERVERPORT, NTHREADS,
                    IMGROOT, LOGLEVEL, HELP
                  };
option::ArgStatus SipiMultiChoice(const option::Option& option, bool msg)
{
    if (option.arg != 0)
    {
        try
        {
            std::string str(option.arg);
            switch(option.index())
            {
            case FORMAT:
                if(str=="jpx" || str=="jpg" || str=="tif" || str=="png") return option::ARG_OK;
            case ICC:
                if(str=="none" || str=="sRGB" || str=="AdobeRGB" || str=="GRAY") return option::ARG_OK;
            case MIRROR:
                if(str=="none" || str=="horizontal" || str=="vertical") return option::ARG_OK;
            case LOGLEVEL:
                if(str=="TRACE" || str=="DEBUG" || str=="INFO" || str=="WARN" || str=="ERROR" || str=="CRITICAL" || str=="OFF") return option::ARG_OK;
            case SKIPMETA:
                if(str=="none" || str=="all") return option::ARG_OK;
            }
        }
        catch(std::exception& e)
        {
            std::cerr<<"Option '"<< option<< "' not a valid argument"<<std::endl;
            return option::ARG_ILLEGAL;
        }
    }

    if (msg) std::cerr<<"Option '"<< option<< "' requires "<<option.desc->help;
    return option::ARG_ILLEGAL;
}
const option::Descriptor usage[] =
{
    {
        UNKNOWN, 0, "", "",option::Arg::None, "USAGE: sipi [options] -ffileIn fileout \n\n"
        "Options:"
    },

    {CONFIGFILE, 0,"c", "config", option::Arg::NonEmpty, "  --config=filename, -cfilename  \tConfiguration file for webserver." },
    {FILEIN, 0,"f", "file", option::Arg::NonEmpty, "  --file=fileIn, -ffileIn  \tinput file to be converted ." },
    {FORMAT, 0,"F", "format", SipiMultiChoice, "  --format, -F  \tOutput format jpx:jpg:tif:png." },
    {ICC, 0,"I", "ICC", SipiMultiChoice, "  --ICC, -I  \tConvert to ICC profile none:sRGB:AdobeRGB:GRAY." },
    {QUALITY, 0, "q", "quality", option::Arg::NumericI, "  --quality, -q  \tQuality (compression) 1:100" },
    {REGION, 0, "r", "region", option::Arg::NonEmpty, "  --region=x,y,w,h, -rx,y,w,h  \tSelect region of interest (x,y,w,h)" },
    {REDUCE, 0, "R", "Reduce", option::Arg::NonEmpty, "  --Reduce, -R  \tReduce image size by factor (Cannot be used together with \"-size\" and \"-scale\"."},
    {SIZE, 0, "s", "size", option::Arg::NonEmpty, "  --size, -s  \tResize image to given size (Cannot be used together with \"-reduce\" and \"-scale\")" },
    {SCALE, 0, "S", "Scale", option::Arg::NonEmpty, "  --Scale, -S  \tResize image by the given percentage (Cannot be used together with \"-size\" and \"-reduce\")" },
    {SKIPMETA, 0, "k", "skipmeta", SipiMultiChoice, "  --skipmeta, -k  \tSkip the given metadata none:all" },
    {MIRROR, 0, "m", "mirror", SipiMultiChoice, "  --mirror, -m  \tMirror the image none:horizontal:vertical" },
    {ROTATE, 0, "o", "rotate", option::Arg::NumericI, "  --rotate, -o  \tRotate the image (0:360)" },
    {SALSAH, 0, "a", "salsah", option::Arg::None, "  --salsah, -s  \tSpecial flag for SALSAH internal use" },
    {COMPARE, 0, "C", "Compare", option::Arg::NonEmpty, "  -Cfile1 -Cfile2, or --Compare=file1 --Compare=file2  \tCompare two files" },
    {SERVERPORT, 0, "p", "serverport", option::Arg::NonEmpty, "  --serverport, -p  \tPort of the webserver" },
    {NTHREADS, 0, "t", "nthreads", option::Arg::NonEmpty, "  --nthreads, -t  \tNumber of threads for webserver" },
    {IMGROOT, 0, "i", "imgroot", option::Arg::NonEmpty, "  --imgroot, -i  \tRoot directory containing the images (webserver)" },
    {LOGLEVEL, 0, "l", "loglevel", SipiMultiChoice, "  --loglevel, -l  \tLogging level TRACE:DEBUG:INFO:WARN:ERROR:CRITICAL:OFF" },
    {HELP, 0,"", "help",option::Arg::None, "  --help  \tPrint usage and exit." },
    {
        UNKNOWN, 0, "", "",option::Arg::None, "\nExamples:\n"
        "  example --unknown -- --this_is_no_option\n"
        "  example -unk --plus -ppp file1 file2\n"
    },
    {0,0,nullptr,nullptr,0,nullptr}
};
int main (int argc, char *argv[]) {
    /*class _SipiInit {
    public:
        _SipiInit() {
            // Initialise libcurl.
            curl_global_init(CURL_GLOBAL_ALL);

            // register namespace sipi in xmp. Since this part of the XMP library is
            // not reentrant, it must be done here in the main thread!
            if (!Exiv2::XmpParser::initialize(Sipi::xmplock_func, &Sipi::xmp_mutex)) {
                std::cerr << "Exiv2::XmpParser::initialize failed" << std::endl;
            }

            Sipi::SipiIOTiff::initLibrary();
        }

        ~_SipiInit() {
            curl_global_cleanup();
        }
    } sipiInit;
*/

    argc-=(argc>0);
    argv+=(argc>0); // skip program name argv[0] if present
    option::Stats  stats(usage, argc, argv);
    std::vector<option::Option> options(stats.options_max);
    std::vector<option::Option> buffer(stats.buffer_max);
    option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

    if (parse.error()){
        return EXIT_FAILURE;
    }else if (options[HELP] || argc == 0)
    {
        option::printUsage(std::cout, usage);
        return EXIT_SUCCESS;
    }else if (options[COMPARE] && options[COMPARE].count()==2)
    {

        std::string infname1,infname2;
        for( option::Option* opt = options[COMPARE]; opt; opt = opt->next())
        {
            try
            {
                if(opt->isFirst())
                {
                    infname1 = std::string(opt->arg);
                }
                else
                {
                    infname2 = std::string(opt->arg);
                }
                std::cout <<"comparing files: " << infname1 <<" and "<< infname2 << std::endl;
            }
            catch(std::logic_error& err)
            {
                std::cerr<<"wrong syntax"<<std::endl;
                return EXIT_FAILURE;
            }
        }

        Sipi::SipiImage img1, img2;
        img1.read(infname1);
        img2.read(infname2);
        bool result = img1 == img2;

        if (!result)
        {
            img1 -= img2;
            img1.write("tif", "diff.tif");
        }

        return (result) ? 0 : -1;
    //
    // if a config file is given, we start sipi as IIIF compatible server
    //
    }else if (options[CONFIGFILE]) {
        std::string configfile;

            try
            {
                configfile = std::string(options[CONFIGFILE].arg);
                std::cout <<"config file: " << configfile << std::endl;
            }
            catch(std::logic_error& err)
            {
                std::cerr<<"  --config=filename, -cfilename  \tConfiguration file for webserver."<<std::endl;
                return EXIT_FAILURE;
            }

        try {
            std::cout << std::endl << SIPI_BUILD_DATE << std::endl;
            std::cout << SIPI_BUILD_VERSION << std::endl;
            //read and parse the config file (config file is a lua script)
            shttps::LuaServer luacfg(configfile);

            //store the config option in a SipiConf obj
            sipiConf = Sipi::SipiConf(luacfg);

            //Create object SipiHttpServer
            Sipi::SipiHttpServer server(sipiConf.getPort(), sipiConf.getNThreads(),
                sipiConf.getUseridStr(), sipiConf.getLogfile(), sipiConf.getLoglevel());

            int old_ll = setlogmask(LOG_MASK(LOG_INFO));
            syslog(LOG_INFO, SIPI_BUILD_DATE);
            syslog(LOG_INFO, SIPI_BUILD_VERSION);
            setlogmask(old_ll);

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
            server.run();

        }
        catch (shttps::Error &err) {
            std::cerr << err << std::endl;
        }
    //
    // if a server port is given, we start sipi as IIIF compatible server on the given port
    //
    }else if(options[SERVERPORT] && options[IMGROOT]) {
        unsigned short nthreads = 0;
        if(options[NTHREADS])
        {
            nthreads = std::stoi(options[NTHREADS].arg);
            if (nthreads < 1 || nthreads > std::thread::hardware_concurrency())
            {
                std::cerr << "incorrect number of threads, maximum supported number is: "<<std::thread::hardware_concurrency() << std::endl;
                nthreads =std::thread::hardware_concurrency();
            }
        }else{
            nthreads =std::thread::hardware_concurrency();
        }

        Sipi::SipiHttpServer server(std::stoi(options[SERVERPORT].arg), nthreads);
        try
        {
            server.imgroot(std::string(options[IMGROOT].arg));
        }
        catch(std::exception& err)
        {
            std::cerr<<"  --imgroot, -i  \tRoot directory containing the images (webserver)"<<std::endl;
            return EXIT_FAILURE;
        }
        serverptr = &server;
        server.run();

    }
    else if(options[FILEIN]){
        //
        // get the input image name
        //
        std::string infname;
        try {
            infname = std::string(options[FILEIN].arg);
        }
        catch (std::exception& err) {
            std::cerr << "incorrect input filename "<<std::endl;
            std::cerr << "USAGE: sipi [options] -ffileIn fileout "<<std::endl;

            return EXIT_FAILURE;
        }

        //
        // get the output image name
        //
        std::string outfname;
        try {
            outfname =  std::string(parse.nonOption(0));
        }
        catch (std::exception& err) {
            std::cerr << "incorrect output filename "<<std::endl;
            std::cerr << "USAGE: sipi [options] -ffileIn fileout "<<std::endl;

            return EXIT_FAILURE;
        }

        //
        // get the output format
        //
        std::string format;
        try {
            format = std::string(options[FORMAT].arg);
        }
        catch (std::exception& err) {
            std::cerr << options[FORMAT].desc->help;
            return EXIT_FAILURE;
        }


        //
        // getting information about a region of interest
        //
        Sipi::SipiRegion *region = NULL;
        if (options[REGION])
        {
            std::vector<int> regV;
            try
            {
                std::stringstream ss(options[REGION].arg);
                std::vector<int> regV;
                int regionC;
                while(ss>>regionC)
                {
                    regV.push_back(regionC);
                    if(ss.peek()==',')
                        ss.ignore();
                }
                if(regV.size()!=4)
                {
                    std::cerr<<"  --region=x,y,w,h, -rx,y,w,h  \tSelect region of interest (x,y,w,h)"<<std::endl;
                    return EXIT_FAILURE;
                }

            }
            catch(std::exception& e)
            {
                std::cerr<<"  --region=x,y,w,h, -rx,y,w,h  \tSelect region of interest (x,y,w,h)"<<std::endl;
                return EXIT_FAILURE;
            }
            region = new Sipi::SipiRegion(regV.at(0),
                                          regV.at(1),
                                          regV.at(2),
                                          regV.at(3));
        }
/*
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
        img.readOriginal(infname, region, size, shttps::HashType::sha256); //convert to bps=8 in case of JPG output
        if (format == "jpg") {
            img.to8bps();
            if (img.getNalpha() > 0) {
                img.removeChan(img.getNc() - 1);
            }
        }
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

        //std::cerr << ">>>> Sipi::SipiIcc rgb_icc = Sipi::SipiIcc(Sipi::icc_AdobeRGB):" << std::endl;
        //Sipi::SipiIcc rgb_icc(Sipi::icc_AdobeRGB);

        //std::cerr << ">>>> img.convertToIcc(rgb_icc, 8):" << std::endl;
        //img.convertToIcc(rgb_icc, 8);

        //std::cout << img << std::endl;
        //std::cerr << ">>>> img.write(" << outfname << "):" << std::endl;

        //
        // write the output file
        //
        try {
            img.write(format, outfname, (params["quality"])[0].getValue (SipiIntType));
        }
        catch (Sipi::SipiImageError &err) {
            std::cerr << err << std::endl;
        }

        if (params["salsah"].isSet()) {
            std::cout << img.getNx() << " " << img.getNy() << std::endl;
        }
*/
    } else{
        option::printUsage(std::cout, usage);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
