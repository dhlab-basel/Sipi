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
#include "Connection.h"

namespace Sipi {

   /*!
    * This class is used to read the sipi server configuration from
    * a Lua configuration file.
    */
   class SipiConf {
   private:

       int port; //<! port number for server
#ifdef SHTTPS_ENABLE_SSL
       int ssl_port = -1;
       std::string ssl_certificate;
       std::string ssl_key;
#endif
       std::string img_root; //<! path to root of image repository
       bool prefix_as_path; //<! Use IIIF-prefix as part of path or ignore it...
       std::string init_script;
       std::string cache_dir;
       std::string cache_size;
       float cache_hysteresis;
       int keep_alive;
       std::string thumb_size;
       int cache_n_files;
       int n_threads;
       std::string tmp_dir;
       std::string scriptdir;
       std::vector<shttps::LuaRoute> routes;
       std::string knora_path;
       std::string knora_port;
       std::string docroot;
       std::string docroute;
   public:
       SipiConf();
       SipiConf(shttps::LuaServer& luacfg);

       inline int getPort(void) { return port; }

#ifdef SHTTPS_ENABLE_SSL
       inline int getSSLPort(void) { return ssl_port; }

       inline std::string getSSLCertificate(void) { return ssl_certificate; }

       inline std::string getSSLKey(void) { return ssl_key; }
#endif

       inline std::string getImgRoot(void) { return img_root; }

       inline bool getPrefixAsPath(void) { return prefix_as_path; }

       inline int getSubdir(void) { return port; }

       inline std::string getInitScript(void) { return init_script; }

       inline std::string getCacheSize(void) { return cache_size; }

       inline std::string getCacheDir(void) { return cache_dir; }

       inline float getCacheHysteresis(void) { return cache_hysteresis; }

       inline int getKeepAlive(void) { return keep_alive; }

       inline std::string getThumbSize(void) { return thumb_size; }

       inline int getCacheNFiles(void) { return cache_n_files; }

       inline int getNThreads(void) { return n_threads; }

       inline std::string getTmpDir(void) { return tmp_dir; }

       inline std::string getScriptDir(void) { return scriptdir; }

       inline std::vector<shttps::LuaRoute> getRoutes(void) { return routes; }

       inline std::string getKnoraPath(void) { return knora_path; }

       inline std::string getKnoraPort(void) { return knora_port; }

       inline std::string getDocRoot(void) { return docroot; }

       inline std::string getDocRoute(void) { return docroute; }

   };

}


#endif
