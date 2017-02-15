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
#include <sys/stat.h>

#include "curl/curl.h"
#include "shttps/Global.h"
#include "shttps/LuaServer.h"
#include "shttps/LuaSqlite.h"
#include "SipiLua.h"
#include "SipiImage.h"
#include "SipiHttpServer.h"
#include "optionparser.h"

#include "jansson.h"
#include "shttps/Parsing.h"
#include "SipiConf.h"

// A macro for silencing incorrect compiler warnings about unused variables.
#define _unused(x) ((void)(x))

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

static const char __file__[] = __FILE__;

static void sipiConfGlobals(lua_State *L, shttps::Connection &conn, void *user_data) {
    Sipi::SipiConf *conf = (Sipi::SipiConf *) user_data;

    lua_createtable(L, 0, 14); // table1

    lua_pushstring(L, "hostname"); // table1 - "index_L1"
    lua_pushstring(L, conf->getHostname().c_str());
    lua_rawset(L, -3); // table1

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

enum optionIndex {
    UNKNOWN, CONFIGFILE, FILEIN, FORMAT, ICC, QUALITY, REGION, REDUCE, SIZE, SCALE,
    SKIPMETA, MIRROR, ROTATE, SALSAH, WATERMARK, COMPARE, SERVERPORT, NTHREADS,
    IMGROOT, LOGLEVEL, HELP
};

option::ArgStatus SipiMultiChoice(const option::Option &option, bool msg) {
    if (option.arg != 0) {
        try {
            std::string str(option.arg);
            switch (option.index()) {
                case FORMAT:
                    if (str == "jpx" || str == "jpg" || str == "tif" || str == "png") return option::ARG_OK;

                    break;
                case ICC:
                    if (str == "none" || str == "sRGB" || str == "AdobeRGB" || str == "GRAY") return option::ARG_OK;
                    break;
                case MIRROR:
                    if (str == "none" || str == "horizontal" || str == "vertical") return option::ARG_OK;
                    break;
                case LOGLEVEL:
                    if (str == "TRACE" || str == "DEBUG" || str == "INFO" || str == "WARN" || str == "ERROR" ||
                        str == "CRITICAL" || str == "OFF")
                        return option::ARG_OK;
                    break;
                case SKIPMETA:
                    if (str == "none" || str == "all") return option::ARG_OK;
                    break;
                default:
                    return option::ARG_ILLEGAL;
            }
        }
        catch (std::exception &e) {
            std::cerr << "Option '" << option << "' not a valid argument" << std::endl;
            return option::ARG_ILLEGAL;
        }
    }


    if (msg) std::cerr << "Option '" << option << "' requires " << option.desc->help;
    return option::ARG_ILLEGAL;
}

const option::Descriptor usage[] =
        {
                {
                        UNKNOWN,    0, "",      "",           option::Arg::None,     "Sipi (Simple Image Presentation Interface)\nSipi is developed by the Digital Humanities Lab at the University of Basel\n"
                                                                                             "USAGE : sipi [options]\n"
                                                                                             "Options:"
                },

                {       CONFIGFILE, 0, "c",     "config",     option::Arg::NonEmpty, "  --config filename, -c filename  \tConfiguration file for web server.\n"},
                {       FILEIN,     0, "f",     "file",       option::Arg::NonEmpty, "  --file fileIn, -f fileIn  \tinput file to be converted. Usage: sipi [options] -f fileIn fileout\n"},
                {       FORMAT,     0, "F",     "format",     SipiMultiChoice,       "  --format Value, -F Value  \tOutput format Value can be: jpx,jpg,tif,png.\n"},
                {       ICC,        0, "I",     "ICC",        SipiMultiChoice,       "  --ICC Value, -I Value  \tConvert to ICC profile. Value can be: none,sRGB,AdobeRGB,GRAY.\n"},
                {       QUALITY,    0, "q",     "quality",    option::Arg::NumericI, "  --quality Value, -q Value  \tQuality (compression). Value can any integer between 1 and 100\n"},
                {       REGION,     0, "r",     "region",     option::Arg::NonEmpty, "  --region x,y,w,h, -r x,y,w,h  \tSelect region of interest, where x,y,w,h are integer values\n"},
                {       REDUCE,     0, "R",     "Reduce",     option::Arg::NumericI, "  --Reduce Value, -R Value  \tReduce image size by factor Value (cannot be used together with --size and --scale)\n"},
                {       SIZE,       0, "s",     "size",       option::Arg::NonEmpty, "  --size w,h -s w,h  \tResize image to given size w,h (cannot be used together with --reduce and --scale)\n"},
                {       SCALE,      0, "S",     "Scale",      option::Arg::NonEmpty, "  --Scale Value, -S Value  \tResize image by the given percentage Value (cannot be used together with --size and --reduce)\n"},
                {       SKIPMETA,   0, "k",     "skipmeta",   SipiMultiChoice,       "  --skipmeta Value, -k Value  \tSkip the given metadata. Value can be none,all\n"},
                {       MIRROR,     0, "m",     "mirror",     SipiMultiChoice,       "  --mirror Value, -m Value  \tMirror the image. Value can be: none,horizontal,vertical\n"},
                {       ROTATE,     0, "o",     "rotate",     option::Arg::NumericD, "  --rotate Value, -o Value  \tRotate the image. by degree Value, angle between (0:360)\n"},
                {       SALSAH,     0, "a",     "salsah",     option::Arg::None,     "  --salsah, -s  \tSpecial flag for SALSAH internal use\n"},
                {       COMPARE,    0, "C",     "Compare",    option::Arg::NonEmpty, "  --Compare file1 --Compare file2 or -C file1 -C file2  \tCompare two files\n"},
                {       WATERMARK,  0, "w",     "watermark",  option::Arg::NonEmpty, "  --watermark file, -w file  \tAdd a watermark to the image\n"},
                {       SERVERPORT, 0, "p",     "serverport", option::Arg::NonEmpty, "  --serverport Value, -p Value  \tPort of the web server\n"},
                {       NTHREADS,   0, "t",     "nthreads",   option::Arg::NonEmpty, "  --nthreads Value, -t Value  \tNumber of threads for web server\n"},
                {       IMGROOT,    0, "i",     "imgroot",    option::Arg::NonEmpty, "  --imgroot Value, -i Value  \tRoot directory containing the images for the web server\n"},
                {       LOGLEVEL,   0, "l",     "loglevel",   SipiMultiChoice,       "  --loglevel Value, -l Value  \tLogging level Value can be: TRACE,DEBUG,INFO,WARN,ERROR,CRITICAL,OFF\n"},
                {       HELP,       0, "",      "help",       option::Arg::None,     "  --help  \tPrint usage and exit.\n"},
                {
                        UNKNOWN,    0, "",      "",           option::Arg::None,     "\nExamples:\n"
                                                                                             "USAGE (server): sipi --config filename or sipi --c filename where filename is a properly formatted configuration file in Lua\n"
                                                                                             "USAGE (server): sipi [options]\n"
                                                                                             "USAGE (image converter): sipi [options] -f fileIn fileout \n"
                                                                                             "USAGE (image diff): sipi --Compare file1 --Compare file2 oor sipi --C file1 -C file2 \n\n"
                },
                {       0,          0, nullptr, nullptr,      0,                     nullptr}
        };

//small function to check if file exist
inline bool exists_file(const std::string &name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

/*!
 * A singleton that does global initialisation and cleanup of libraries used by Sipi. This class is used only
 * in main().
 */
class LibraryInitialiser {
public:
    /*!
     * @return the singleton instance.
     */
    static LibraryInitialiser &instance() {
        // In C++11, initialization of this static local variable happens once and is thread-safe.
        static LibraryInitialiser sipi_init;
        return sipi_init;
    }

private:
    LibraryInitialiser() {
        // Initialise libcurl.
        curl_global_init(CURL_GLOBAL_ALL);

        // Initialise Exiv2, registering namespace sipi. Since this is not thread-safe, it must
        // be done here in the main thread.
        if (!Exiv2::XmpParser::initialize(Sipi::xmplock_func, &Sipi::xmp_mutex)) {
            throw shttps::Error(__file__, __LINE__, "Exiv2::XmpParser::initialize failed");
        }

        // Inititalise the TIFF library.
        Sipi::SipiIOTiff::initLibrary();
    }

    ~LibraryInitialiser() {
        // Clean up libcurl.
        curl_global_cleanup();

        // Clean up Exiv2.
        Exiv2::XmpParser::terminate();
    }
};

int main(int argc, char *argv[]) {
    try {
        // Initialise libraries used by Sipi.
        LibraryInitialiser &sipi_init = LibraryInitialiser::instance();
        _unused(sipi_init); // Silence compiler warning about unused variable.
    } catch (shttps::Error &e) {
        std::cerr << e.to_string() << std::endl;
        return EXIT_FAILURE;
    }

    argc -= (argc > 0);
    argv += (argc > 0); // skip program name argv[0] if present

    option::Stats stats(usage, argc, argv);
    std::vector<option::Option> options(stats.options_max);
    std::vector<option::Option> buffer(stats.buffer_max);
    option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

    if (parse.error()) {
        std::cout << "##" << __LINE__ << std::endl;
        option::printUsage(std::cout, usage);
        return EXIT_FAILURE;
    } else if (options[HELP] || argc == 0) {
        std::cout << "##" << __LINE__ << std::endl;
        option::printUsage(std::cout, usage);
        return EXIT_SUCCESS;
    } else if (options[COMPARE] && options[COMPARE].count() == 2) {

        std::string infname1, infname2;
        for (option::Option *opt = options[COMPARE]; opt; opt = opt->next()) {
            try {
                if (opt->isFirst()) {
                    infname1 = std::string(opt->arg);
                } else {
                    infname2 = std::string(opt->arg);
                }
                std::cout << "comparing files: " << infname1 << " and " << infname2 << std::endl;
            }
            catch (std::exception &err) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << options[COMPARE].desc->help << std::endl;
                return EXIT_FAILURE;
            }
        }

        if (!exists_file(infname1)) {
            std::cout << "##" << __LINE__ << std::endl;
            std::cerr << "File not found: " << infname1 << std::endl;
            std::cerr << options[FILEIN].desc->help << std::endl;
            return EXIT_FAILURE;
        }

        if (!exists_file(infname2)) {
            std::cout << "##" << __LINE__ << std::endl;
            std::cerr << "File not found: " << infname2 << std::endl;
            std::cerr << options[FILEIN].desc->help << std::endl;
            return EXIT_FAILURE;
        }

        Sipi::SipiImage img1, img2;
        img1.read(infname1);
        img2.read(infname2);
        bool result = img1 == img2;

        if (!result) {
            img1 -= img2;
            img1.write("tif", "diff.tif");
        }

        return (result) ? 0 : -1;
        //
        // if a config file is given, we start sipi as IIIF compatible server
        //
    } else if (options[CONFIGFILE]) {
        std::string configfile;

        try {
            configfile = std::string(options[CONFIGFILE].arg);
            // std::cout << "Config file: " << configfile << std::endl;
        }
        catch (std::logic_error &err) {
            std::cerr << options[CONFIGFILE].desc->help << std::endl;
            return EXIT_FAILURE;
        }

        if (!exists_file(configfile)) {
            std::cout << "##" << __LINE__ << std::endl;
            std::cerr << "File not found: " << configfile << std::endl;
            std::cerr << options[CONFIGFILE].desc->help << std::endl;
            return EXIT_FAILURE;
        }

        try {
            std::cout << std::endl << SIPI_BUILD_DATE << std::endl;
            std::cout << SIPI_BUILD_VERSION << std::endl;
            //read and parse the config file (config file is a lua script)
            shttps::LuaServer luacfg(configfile);

            //store the config option in a SipiConf obj
            Sipi::SipiConf sipiConf(luacfg);

            //Create object SipiHttpServer
            Sipi::SipiHttpServer server(sipiConf.getPort(), static_cast<unsigned int> (sipiConf.getNThreads()),
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

            server.scriptdir(
                    sipiConf.getScriptDir()); // set the directory where the Lua scripts are found for the "Lua"-routes
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
                        cachesize = stoll(cachesize_str.substr(0, l - 1)) * 1024 * 1024;
                    } else if (c == 'G') {
                        cachesize = stoll(cachesize_str.substr(0, l - 1)) * 1024 * 1024 * 1024;
                    } else {
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
            std::pair<std::string, std::string> filehandler_info;

            if (!(docroute.empty() || docroot.empty())) {
                filehandler_info.first = docroute;
                filehandler_info.second = docroot;
                server.addRoute(shttps::Connection::GET, docroute, shttps::FileHandler, &filehandler_info);
                server.addRoute(shttps::Connection::POST, docroute, shttps::FileHandler, &filehandler_info);
            }

            server.run();

        }
        catch (shttps::Error &err) {
            std::cerr << err << std::endl;
        }
        //
        // if a server port is given, we start sipi as IIIF compatible server on the given port
        //
    } else if (options[SERVERPORT] && options[IMGROOT]) {
        unsigned int nthreads = 0;
        if (options[NTHREADS]) {
            nthreads = static_cast<unsigned int> (std::stoi(options[NTHREADS].arg));
            if (nthreads < 1 || nthreads > std::thread::hardware_concurrency()) {
                std::cerr << "incorrect number of threads, maximum supported number is: "
                          << std::thread::hardware_concurrency() << std::endl;
                nthreads = std::thread::hardware_concurrency();
            }
        } else {

            nthreads = std::thread::hardware_concurrency();
        }

        Sipi::SipiHttpServer server(std::stoi(options[SERVERPORT].arg), nthreads);

        try {
            server.imgroot(std::string(options[IMGROOT].arg));
        }
        catch (std::exception &err) {
            std::cout << "##" << __LINE__ << std::endl;
            std::cerr << options[IMGROOT].desc->help << std::endl;
            return EXIT_FAILURE;
        }

        server.run();

    } else if (options[FILEIN]) {

        //
        // get the input image name
        //
        std::string infname;
        try {
            infname = std::string(options[FILEIN].arg);
        }
        catch (std::exception &err) {
            std::cout << "##" << __LINE__ << std::endl;
            std::cerr << "Invalid input filename." << std::endl;
            std::cerr << options[FILEIN].desc->help << std::endl;
            return EXIT_FAILURE;
        }

        if (!exists_file(infname)) {
            std::cout << "##" << __LINE__ << std::endl;
            std::cerr << "File not found: " << infname << std::endl;
            std::cerr << options[FILEIN].desc->help << std::endl;
            return EXIT_FAILURE;
        }

        //
        // get the output image name
        //
        std::string outfname("out.jpx");

        if (parse.nonOptionsCount() > 0) {
            try {
                outfname = std::string(parse.nonOption(0));
            }
            catch (std::exception &err) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << "incorrect output filename " << std::endl;
                std::cerr << options[FILEIN].desc->help << std::endl;
                return EXIT_FAILURE;
            }
        } else {
            std::cerr << "missing output filename, using default out.jpx " << std::endl;
            std::cerr << options[FILEIN].desc->help << std::endl;
        }

        //
        // get the output format
        //
        std::string format("jpx");

        if (options[FORMAT]) {
            try {
                format = std::string(options[FORMAT].arg);
            }
            catch (std::exception &err) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << options[FORMAT].desc->help;
                return EXIT_FAILURE;
            }
        }

        //
        // getting information about a region of interest
        //
        std::shared_ptr<Sipi::SipiRegion> region;

        if (options[REGION]) {
            std::vector<int> regV;
            try {
                std::stringstream ss(options[REGION].arg);
                int regionC;
                while (ss >> regionC) {
                    regV.push_back(regionC);
                    if (ss.peek() == ',') {
                        ss.ignore();
                    }
                }
                if (regV.size() != 4) {
                    std::cout << "##" << __LINE__ << std::endl;
                    std::cerr << options[REGION].desc->help << std::endl;
                    return EXIT_FAILURE;
                }

            }
            catch (std::exception &e) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << options[REGION].desc->help << std::endl;
                return EXIT_FAILURE;
            }
            region = std::make_shared<Sipi::SipiRegion>(regV.at(0),
                                                        regV.at(1),
                                                        regV.at(2),
                                                        regV.at(3));
        }

        std::shared_ptr<Sipi::SipiSize> size;

        //
        // get the reduce parameter
        // "reduce" is a special feature of the JPEG2000 format. It is possible (given the JPEG2000 format
        // is written a resolution pyramid). reduce=0 results in full resolution, reduce=1 is half the resolution
        // etc.
        //
        int reduce;
        try {
            reduce = options[REDUCE] ? (std::stoi(options[REDUCE].arg)) : 0;
        }
        catch (std::exception &e) {
            std::cout << "##" << __LINE__ << std::endl;
            std::cerr << options[REDUCE].desc->help << std::endl;
            return EXIT_FAILURE;
        }
        if (reduce > 0) {
            size = std::make_shared<Sipi::SipiSize>(reduce);
        } else if (options[SIZE]) {
            try {
                size = std::make_shared<Sipi::SipiSize>(options[SIZE].arg);
            }
            catch (std::exception &e) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << options[SIZE].desc->help << std::endl;
                return EXIT_FAILURE;
            }
        } else if (options[SCALE]) {
            try {
                size = std::make_shared<Sipi::SipiSize>(std::stoi(options[SCALE].arg));
            }
            catch (std::exception &e) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << options[SCALE].desc->help << std::endl;
                return EXIT_FAILURE;
            }
        }

        //
        // read the input image
        //
        Sipi::SipiImage img;
        img.readOriginal(infname, region, size, shttps::HashType::sha256); //convert to bps=8 in case of JPG output
        if (format == "jpg") {
            img.to8bps();
            if (img.getNalpha() > 0) {
                img.removeChan(static_cast<unsigned int>(img.getNc() - 1));
            }
        }

        //
        // if we want to remove all metadata from the file...
        //
        std::string skipmeta("none");
        if (options[SKIPMETA]) {
            try {
                skipmeta = options[SKIPMETA].arg;
            }
            catch (std::exception &e) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << options[SKIPMETA].desc->help << std::endl;
                return EXIT_FAILURE;
            }
        }
        if (skipmeta != "none") {
            img.setSkipMetadata(Sipi::SKIP_ALL);
        }

        //
        // color profile processing
        //

        std::string iccprofile("none");
        if (options[ICC]) {
            try {
                iccprofile = options[ICC].arg;
            }
            catch (std::exception &e) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << options[ICC].desc->help << std::endl;
                return EXIT_FAILURE;
            }
        }

        if (iccprofile != "none") {
            Sipi::SipiIcc icc;
            if (iccprofile == "sRGB") {
                icc = Sipi::SipiIcc(Sipi::icc_sRGB);
            } else if (iccprofile == "AdobeRGB") {
                icc = Sipi::SipiIcc(Sipi::icc_AdobeRGB);
            } else if (iccprofile == "GRAY") {
                icc = Sipi::SipiIcc(Sipi::icc_GRAY_D50);
            } else {
                icc = Sipi::SipiIcc(Sipi::icc_sRGB);
            }
            img.convertToIcc(icc, 8);
        }

        //
        // mirroring and rotation
        //
        std::string mirror("none");
        if (options[MIRROR]) {
            try {
                mirror = options[MIRROR].arg;
            }
            catch (std::exception &e) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << options[MIRROR].desc->help << std::endl;
                return EXIT_FAILURE;
            }
        }
        float angle = 0.0F;
        if (options[ROTATE]) {
            try {
                angle = std::stof(options[ROTATE].arg);
            }
            catch (std::exception &e) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << options[ROTATE].desc->help << std::endl;
                return EXIT_FAILURE;
            }
        }

        if (mirror != "none") {
            if (mirror == "horizontal") {
                img.rotate(angle, true);
            } else if (mirror == "vertical") {
                angle += 180.0F;
                img.rotate(angle, true);
            } else {
                img.rotate(angle, false);
            }
        } else if (angle != 0.0F) {
            img.rotate(angle, false);
        }

        if (options[WATERMARK]) {
            std::string infname;
            try {
                infname = std::string(options[WATERMARK].arg);
            }
            catch (std::exception &err) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << "Invalid watermark filename." << std::endl;
                std::cerr << options[WATERMARK].desc->help << std::endl;
                return EXIT_FAILURE;
            }

            if (!exists_file(infname)) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << "File not found: " << infname << std::endl;
                std::cerr << options[WATERMARK].desc->help << std::endl;
                return EXIT_FAILURE;
            }
            img.add_watermark(infname);
        }

        //
        // write the output file
        //
        int quality = 80;
        if (options[QUALITY]) {
            try {
                quality = std::stoi(options[QUALITY].arg);
            }
            catch (std::exception &e) {
                std::cout << "##" << __LINE__ << std::endl;
                std::cerr << options[QUALITY].desc->help << std::endl;
                return EXIT_FAILURE;
            }
        }

        try {
            img.write(format, outfname, quality);
        }
        catch (Sipi::SipiImageError &err) {
            std::cerr << err << std::endl;
        }

        if (options[SALSAH]) {
            std::cout << img.getNx() << " " << img.getNy() << std::endl;
        }
    } else {
        std::cout << "##" << __LINE__ << std::endl;
        option::printUsage(std::cout, usage);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
