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
#ifndef __shttp_server_h
#define __shttp_server_h

#include <map>
#include <vector>
#include <mutex>
#include <csignal>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h> //for threading , link with lpthread
#include <semaphore.h>
#include <netdb.h>      // Needed for the socket functions
#include <sstream>      // std::stringstream

#include "spdlog/spdlog.h"  // logging...

#ifdef SHTTPS_ENABLE_SSL
#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#endif

#include "Global.h"
#include "Connection.h"
#include "LuaServer.h"

#include "lua.hpp"

/*
 * How to create a self-signed certificate
 *
 * openssl genrsa -out key.pem 2048
 * openssl req -new -key key.pem -out csr.pem
 * openssl req -x509 -days 365 -key key.pem -in csr.pem -out certificate.pem
 */

namespace shttps {


    typedef void (*RequestHandler)(Connection &, LuaServer &, void *, void *);

    extern void FileHandler(shttps::Connection &conn, LuaServer &lua, void *user_data, void *handler_data);


    /*!
     * \brief Implements a simple, even primitive HTTP server with routes and handlers
     *
     * Implementation of a simple, almost primitive, multithreaded HTTP server. The user can define for
     * request types and different paths handlers which will be called. The handler gets an Connection
     * instance which will be used to receive and send data.
     *
     *     void MirrorHandler(shttps::Connection &conn, void *user_data)
     *     {
     *         conn.setBuffer();
     *         std::vector<std::string> headers = conn.header();
     *         if (!conn.query("html").empty()) {
     *             conn.header("Content-Type", "text/html; charset=utf-8");
     *             conn << "<html><head>";
     *             conn << "<title>Mirror, mirror – on the wall...</title>";
     *             conn << "</head>" << shttps::Connection::flush_data;
     *
     *             conn << "<body><h1>Header fields</h1>";
     *             conn << "<table>";
     *             conn << <tr><th>Fieldname</th><th>Value</th></tr>";
     *             for (unsigned i = 0; i < headers.size(); i++) {
     *               conn << "<tr><td>" << headers[i] << "</td><td>" << conn.header(headers[i]) << "</td></tr>";
     *             }
     *             conn << "</table>"
     *             conn << "</body></html>" << shttps::Connection::flush_data;
     *         }
     *         else {
     *             conn.header("Content-Type", "text/plain; charset=utf-8");
     *             for (unsigned i = 0; i < headers.size(); i++) {
     *                 conn << headers[i] << " : " << conn.header(headers[i]) << "\n";
     *             }
     *         }
     *     }
     *
     *     shttps::Server server(4711);
     *     server.addRoute(shttps::Connection::GET, "/", RootHandler);
     *     server.addRoute(shttps::Connection::GET, "/mirror", MirrorHandler);
     *     server.run();
     *
     */
    class Server {
        /*!
         * Struct to hold Global Lua function and associated userdata
         */
        typedef struct {
            LuaSetGlobalsFunc func;
            void *func_dataptr;
        } GlobalFunc;

        /*!
         * Struct to hold the information about threads and open sockets, used for proper shutdown
         */
        typedef struct {
            int sid;    //!< socket id
#ifdef SHTTPS_ENABLE_SSL
            SSL *ssl_sid; //!< Pointer to SLL socket struct
#endif
        } GenericSockId;

#ifdef SHTTPS_ENABLE_SSL
        /*!
         * Error handling class for SSL functions
         */
        class SSLError : Error {
        protected:
            SSL *cSSL;
        public:
            inline SSLError (const char *file, const int line, const char *msg, SSL *cSSL_p = NULL)
                : Error(file, line, msg), cSSL(cSSL_p) {};
            inline SSLError (const char *file, const int line, const std::string &msg, SSL *cSSL_p = NULL)
                : Error(file, line, msg), cSSL(cSSL_p) {};
            inline std::string to_string(void) {
                std::stringstream ss;
                ss << "SSL-ERROR at [" << file << ": " << line << "] ";
                BIO *bio = BIO_new(BIO_s_mem());
                ERR_print_errors (bio);
                char *buf = NULL;
                long n =  BIO_get_mem_data (bio, &buf);
                if (n > 0) {
                    ss << buf << " : ";
                }
                BIO_free (bio);
                //ss << "Description: " << message;
                return ss.str();
            };
        };
#endif

    public:

    private:
        int port; //!< listening Port for server
        int _ssl_port; //!< listening port for openssl
        int _sockfd; //!< socket id
        int _ssl_sockfd; //!< SSL socket id
#ifdef SHTTPS_ENABLE_SSL
        std::string _ssl_certificate; //!< Path to SSL certificate
        std::string _ssl_key; //!< Path to SSL certificate
        std::string _jwt_secret;
#endif
        std::string _tmpdir; //!< path to directory, where uplaods are being stored
        std::string _scriptdir; //!< Path to directory, thwere scripts for the "Lua"-routes are found
        unsigned _nthreads; //!< maximum number of parallel threads for processing requests
        std::string semname; //!< name of the semaphore for restricting the number of threads
        sem_t *_semaphore; //!< semaphore
        std::map<pthread_t,GenericSockId> thread_ids;
        std::mutex threadlock;
        std::mutex debugio;
        int _keep_alive_timeout;
        bool running;
        std::map<std::string, RequestHandler> handler[9]; // request handlers for the different 9 request methods
        std::map<std::string, void *> handler_data[9]; // request handlers for the different 9 request methods
        void *_user_data; //!< Some opaque user data that can be given to the Connection (for use within the handler)
        std::string _initscript;
        std::vector<shttps::LuaRoute> _lua_routes; //!< This vector holds the routes that are served by lua scripts
        std::vector<GlobalFunc> lua_globals;

        RequestHandler getHandler(Connection &conn, void **handler_data_p);
        std::string _logfilename;
    protected:
        std::shared_ptr<spdlog::logger> _logger;
    public:
        /*!
        * Create a server listening on the given port with the maximal number of threads
        *
        * \param[in] port_p Listening port of HTTP server
        * \param[in] nthreads_p Maximal number of parallel threads serving the requests
        */
        Server(int port_p, unsigned nthreads_p = 4, const std::string userid_str = "", const std::string &logfile_p = "shttps.log");

#ifdef SHTTPS_ENABLE_SSL

        /*!
         * Sets the port number for the SSL socket
         *
         * \param[in] ssl_port_p Port number
         */
        inline void ssl_port(int ssl_port_p) { _ssl_port = ssl_port_p; }

        /*!
         * Gets the port number of the SSL socket
         *
         * \returns SSL socket portnumber
         */
        inline int ssl_port(void) { return _ssl_port; }

        /*!
         * Sets the file path to the SSL certficate necessary for OpenSSL to work
         *
         * \param[in] path File path to th SSL certificate
         */
        inline void ssl_certificate(const std::string &path) { _ssl_certificate = path; }

        /*!
         * Returns the path of the SSL certificate
         *
         * \returns Path to the SSL certificate
         */
        inline std::string ssl_certificate(void) { return _ssl_certificate; }

        /*!
         * Sets the path to the SSP key
         *
         * \param[in] path Path to the SSL key necessary for OpenSSL to work
         */
        inline void ssl_key(const std::string &path) { _ssl_key = path; }

        /*!
         * Returns the path of the OpenSSL key
         *
         * \returns Path to the OpenSSL key
         */
        inline std::string ssl_key(void) { return _ssl_key; }

        /*!
         * Sets the secret for the generation JWT's (JSON Web Token). It must be a string
         * of length 32, since we're using currently SHA256 encoding.
         *
         * \param[in] jwt_secret_p String with 32 characters for the key for JWT's
         */
        void jwt_secret(const std::string &jwt_secret_p);

        /*!
         * Returns the secret used for JWT's
         *
         * \returns String of length 32 with the secret used for JWT's
         */
        inline std::string jwt_secret(void) { return _jwt_secret; }
#endif

        /*!
         * Returns the maximum number of parallel threads allowed
         *
         * \returns Number of parallel threads allowed
         */
        inline unsigned nthreads(void) { return _nthreads; }

        /*!
         * Return the path where to store temporary files (for uploads)
         *
         * \returns Path to directory for temporary files
         */
        inline std::string tmpdir(void) { return _tmpdir; }

        /*!
         * set the path to the  directory where to store temporary files during uploads
         *
         * \param[in] path to directory without trailing '/'
         */
        inline void tmpdir(const std::string &tmpdir_p) { _tmpdir = tmpdir_p; }

        /*!
        * Return the path where the Lua scripts for "Lua"-routes are found
        *
        * \returns Path to directory for script directory
        */
        inline std::string scriptdir(void) { return _scriptdir; }

        /*!
         * set the path to the  directory where to store temporary files during uploads
         *
         * \param[in] path to directory without trailing '/'
         */
        inline void scriptdir(const std::string &scriptdir_p) { _scriptdir = scriptdir_p; }

        /*!
        * Returns the routes defined for being handletd by Lua scripts
        *
        * \returns Vector of Lua route infos
        */
        inline std::vector<shttps::LuaRoute> luaRoutes(void) { return _lua_routes; }

        /*!
         * set the routes that should be handled by Lua scripts
         *
         * \param[in] Vector of lua route infos
         */
        inline void luaRoutes(const std::vector<shttps::LuaRoute> &lua_routes_p) { _lua_routes = lua_routes_p; }

        /*!
        * Set the loglevel
        *
        * \param[in] loglevel_p set the loglevel
        */
        inline void loglevel(spdlog::level::level_enum loglevel_p) {
            spdlog::set_level(loglevel_p);
        }

        /*!
         * Run the server handling requests in an infinite loop
         */
        void run();

        /*!
         * Stop the server gracefully (all destructors are called etc.) and the
         * cache file is updated.
         */
        inline void stop(void) {
            running = false;
            close(_sockfd);
        }

        /*!
         * Set the default value for the keep alive timout. This is the time in seconds
         * a HTTP connection (socket) remains up without action before being closed by
         * the server. A keep-alive header will change this value
         */
        inline void keep_alive_timeout(int keep_alive_timeout) { _keep_alive_timeout = keep_alive_timeout; }

        /*!
         * Returns the default keep alive timeout
         *
         * \returns Keep alive timeout in seconds
         */
        inline int keep_alive_timeout(void) { return _keep_alive_timeout; }

        /*!
         * Return the internal semaphore structure
         *
         * \returns The semaphore controlling the maximal number of threads
         */
        inline sem_t *semaphore(void) { return _semaphore; }

        inline void add_thread(pthread_t thread_id_p, int sock_id) {
            threadlock.lock();
            GenericSockId sid;
            sid.sid = sock_id;
#ifdef SHTTPS_ENABLE_SSL
            sid.ssl_sid = NULL;
#endif
            thread_ids[thread_id_p] = sid;
            threadlock.unlock();
        }

#ifdef SHTTPS_ENABLE_SSL
        inline void add_thread(pthread_t thread_id_p, int sock_id, SSL *cSSL) {
            threadlock.lock();
            GenericSockId sid = {sock_id, cSSL};
            thread_ids[thread_id_p] = sid;
            threadlock.unlock();
        }
#endif

        inline void remove_thread(pthread_t thread_id_p) {
            threadlock.lock();
            thread_ids.erase(thread_id_p);
            threadlock.unlock();
        }

        /*!
         * Sets the path to the initialization script (lua script) which is executed for each request
         *
         * \param[in] initscript_p Path of initialization script
         */
        inline void initscript(const std::string &initscript_p) { _initscript = initscript_p; }

        /*!
         * adds a function which is called before processing each request to initialize
         * special Lua variables and add special Lua functions
         *
         * \param[in] func C++ function which extends the Lua
         */
        inline void add_lua_globals_func(LuaSetGlobalsFunc func, void *user_data = NULL) {
            GlobalFunc gf;
            gf.func = func;
            gf.func_dataptr = user_data;
            lua_globals.push_back(gf);
        }

        /*!
         * Add a request handler for the given request method and route
         *
         * \param[in] method_p Request method (GET, PUT, POST etc.)
         * \param[in] path Route that this handler should serve
         * \param[in] handler_p Handler function which serves this method/route combination.
         *            The handler has the form
         *
         *      void (*RequestHandler)(Connection::Connection &, void *);
         *
         * \param[in] handler_data_p Pointer to arbitrary data given to the handler when called
         *
         */
        void addRoute(Connection::HttpMethod method_p, const std::string &path, RequestHandler handler_p,
                      void *handler_data_p = NULL);

       /*!
        * Process a request... (Eventually should be private method)
        *
        * \param[in] sock Socket id
        * \param[in] peer_ip String containing IP (IP4 or IP6) of client/peer
        * \param[in] peer_port Port number of peer/client
        */
        bool processRequest(int sock, std::istream *ins, std::ostream *os, std::string &peer_ip, int peer_port, bool secure);

        /*!
        * Return the user data that has been added previously
        */
        inline void *user_data(void) { return _user_data; }

        /*!
        * Add a pointer to user data which will be made available to the handler
        *
        * \param[in] User data
        */
        inline void user_data(void *user_data_p) { _user_data = user_data_p; }

        inline void debugmsg(const std::string &msg) {
            debugio.lock();
            std::cerr << msg << std::endl;
            debugio.unlock();
        }
    };

}

#endif
