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
/*!
 * \ Handler of various
 *
 */
#ifndef __sipi_conf_h
#define __sipi_conf_h

#include "shttps/LuaServer.h"
#include "shttps/Connection.h"

namespace Sipi {

    /*!
     * This class is used to read the sipi server configuration from
     * a Lua configuration file.
     */
    class SipiConf {
    private:
        std::string userid_str;
        std::string hostname;
        int port; //<! port number for server
#ifdef SHTTPS_ENABLE_SSL
        int ssl_port = -1;
        std::string ssl_certificate;
        std::string ssl_key;
#endif
        std::string img_root; //<! path to root of image repository
        int subdir_levels = -1;
        std::vector<std::string> subdir_excludes;
        bool prefix_as_path; //<! Use IIIF-prefix as part of path or ignore it...
        int jpeg_quality;
        std::map<std::string,std::string> scaling_quality;
        std::string init_script;
        std::string cache_dir;
        size_t cache_size;
        float cache_hysteresis;
        int keep_alive;
        std::string thumb_size;
        int cache_n_files;
        int n_threads;
        size_t max_post_size;
        std::string tmp_dir;
        std::string scriptdir;
        std::vector<shttps::LuaRoute> routes;
        std::string knora_path;
        std::string knora_port;
        std::string logfile;
        std::string loglevel;
        std::string docroot;
        std::string wwwroute;
        std::string jwt_secret;
        std::string adminuser;
        std::string password;

    public:
        SipiConf();

        SipiConf(shttps::LuaServer &luacfg);

        inline std::string getUseridStr(void) { return userid_str; }

        inline std::string getHostname(void) { return hostname; }

        inline int getPort(void) { return port; }

#ifdef SHTTPS_ENABLE_SSL

        inline int getSSLPort(void) { return ssl_port; }

        inline std::string getSSLCertificate(void) { return ssl_certificate; }

        inline std::string getSSLKey(void) { return ssl_key; }

#endif

        inline std::string getImgRoot(void) { return img_root; }

        inline bool getPrefixAsPath(void) { return prefix_as_path; }

        inline int getJpegQuality(void) { return jpeg_quality; }

        inline std::map<std::string,std::string> getScalingQuality(void) { return scaling_quality; }

        inline int getSubdirLevels(void) { return subdir_levels; }

        inline std::vector<std::string> getSubdirExcludes(void) { return subdir_excludes; }

        inline std::string getInitScript(void) { return init_script; }

        inline size_t getCacheSize(void) { return cache_size; }

        inline std::string getCacheDir(void) { return cache_dir; }

        inline float getCacheHysteresis(void) { return cache_hysteresis; }

        inline int getKeepAlive(void) { return keep_alive; }

        inline std::string getThumbSize(void) { return thumb_size; }

        inline int getCacheNFiles(void) { return cache_n_files; }

        inline int getNThreads(void) { return n_threads; }

        inline size_t getMaxPostSize(void) { return max_post_size; }

        inline std::string getTmpDir(void) { return tmp_dir; }

        inline std::string getScriptDir(void) { return scriptdir; }

        inline std::vector<shttps::LuaRoute> getRoutes(void) { return routes; }

        inline std::string getKnoraPath(void) { return knora_path; }

        inline std::string getKnoraPort(void) { return knora_port; }

        inline std::string getLoglevel(void) { return loglevel; }

        inline std::string getLogfile(void) { return logfile; }

        inline std::string getDocRoot(void) { return docroot; }

        inline std::string getWWWRoute(void) { return wwwroute; }

        inline std::string getJwtSecret(void) { return jwt_secret; }

        inline std::string getAdminUser(void) { return adminuser; }

        inline std::string getPassword(void) { return password; }
    };

}


#endif
