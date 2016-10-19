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
 * \file Connection.cpp
 */
#include <algorithm>
#include <functional>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>      // Needed for memset
#include <utility>

#include <sys/types.h>
#include <sys/select.h>
#include <signal.h>
#include <poll.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>

//
// openssl includes
//#include "openssl/applink.c"


#include "Global.h"
#include "SockStream.h"
#include "Server.h"
#include "LuaServer.h"
#include "GetMimetype.h"

static const char __file__[] = __FILE__;

static std::mutex threadlock; // mutex to protect map of active threads
static std::mutex idlelock; // mutex to protect vector of idle threads (keep alive condition)
static std::mutex debugio; // mutex to protect debugging messages from threads

static std::vector<pthread_t> idle_thread_ids;


using namespace std;

namespace shttps {

    const char loggername[] = "shttps-logger"; // see Global.h !!

    typedef struct {
        int sock;
#ifdef SHTTPS_ENABLE_SSL
        SSL *cSSL;
#endif
        string peer_ip;
        int peer_port;
        int commpipe_read;
        Server *serv;
    } TData;
    //=========================================================================


    static void default_handler(Connection &conn, LuaServer &lua, void *user_data, void *hd)
    {
        conn.status(Connection::NOT_FOUND);
        conn.header("Content-Type", "text/text");
        conn.setBuffer();
        try {
            conn << "No handler available" << Connection::flush_data;
        }
        catch (int i) {
            return;
        }
        auto logger = spdlog::get(loggername);
        logger->warn("No handler available! Host: ") << conn.host() << " Uri: " << conn.uri();
        return;
    }
    //=========================================================================


    void ScriptHandler(shttps::Connection &conn, LuaServer &lua, void *user_data, void *hd)
    {
        std::vector<std::string> headers = conn.header();
        std::string uri = conn.uri();

        auto logger = spdlog::get(loggername);

        string script = *((string *) hd);

        if (access(script.c_str(), R_OK) != 0) { // test, if file exists
            conn.status(Connection::NOT_FOUND);
            conn.header("Content-Type", "text/text; charset=utf-8");
            conn << "File not found\n";
            conn.flush();
            logger->error("ScriptHandler: \"") << script << "\" not readable!";
            return;
        }

        size_t extpos = script.find_last_of('.');
        std::string extension;
        if (extpos != std::string::npos) {
            extension = script.substr(extpos + 1);
        }

        try {
            if (extension == "lua") { // pure lua
                ifstream inf;
                inf.open(script);//open the input file

                stringstream sstr;
                sstr << inf.rdbuf();//read the file
                string luacode = sstr.str();//str holds the content of the file

                try {
                    if (lua.executeChunk(luacode) != 1) {
                        conn.flush();
                        return;
                    }
                }
                catch (Error &err) {
                    try {
						conn.setBuffer();
                        conn.status(Connection::INTERNAL_SERVER_ERROR);
                        conn.header("Content-Type", "text/text; charset=utf-8");
                        conn << "Lua Error:\r\n==========\r\n" << err << "\r\n";
                        conn.flush();
                    }
                    catch(int i) {
                        return;
                    }
                    logger->error("ScriptHandler: error executing lua script") << err;
                    return;
                }
                conn.flush();
            }
            else if (extension == "elua") { // embedded lua <lua> .... </lua>
                conn.setBuffer();
                ifstream inf;
                inf.open(script);//open the input file

                stringstream sstr;
                sstr << inf.rdbuf();//read the file
                string eluacode = sstr.str(); // eluacode holds the content of the file

                size_t pos = 0;
                size_t end = 0; // end of last lua code (including </lua>)
                while ((pos = eluacode.find("<lua>", end)) != string::npos) {
                    string htmlcode = eluacode.substr(end, pos - end);
                    pos += 5;

                    if (!htmlcode.empty()) conn << htmlcode; // send html...

                    string luastr;
                    if ((end = eluacode.find("</lua>", pos)) != string::npos) { // we found end;
                        luastr = eluacode.substr(pos, end - pos);
                        end += 6;
                    }
                    else {
                        luastr = eluacode.substr(pos);
                    }

                    try {
                        if (lua.executeChunk(luastr) != 1) {
                            conn.flush();
                            return;
                        }
                    }
                    catch (Error &err) {
                        try {
                            conn.status(Connection::INTERNAL_SERVER_ERROR);
                            conn.header("Content-Type", "text/text; charset=utf-8");
                            conn << "Lua Error:\r\n==========\r\n" << err << "\r\n";
                            conn.flush();
                        }
                        catch (int i) {
                            return;
                        }
                        logger->error("ScriptHandler: error executing lua chunk! ") << err;
                        return;
                    }
                }
                string htmlcode = eluacode.substr(end);
                conn << htmlcode;
                conn.flush();
            }
            else {
                conn.status(Connection::INTERNAL_SERVER_ERROR);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << "Script has no valid extension: '" << extension << "' !";
                conn.flush();
                logger->error("ScriptHandler: error executing script, unknown extension: '") << extension << "' !";
            }
        }
        catch (int i) {
            return; // we have an io error => just return, the thread will exit
        }
        catch (Error &err) {
            try {
                conn.status(Connection::INTERNAL_SERVER_ERROR);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << err;
                conn.flush();
            }
            catch (int i) {
                return;
            }
            logger->error("FileHandler: internal error: ") << err;
            return;
        }
    }
    //=========================================================================


    void FileHandler(shttps::Connection &conn, LuaServer &lua, void *user_data, void *hd)
    {
        std::vector<std::string> headers = conn.header();
        std::string uri = conn.uri();

        auto logger = spdlog::get(loggername);

        string docroot;
        string route;
        if (hd == NULL) {
            docroot = ".";
            route = "/";
        }
        else {
            pair<string,string> tmp = *((pair<string,string> *)hd);
            docroot = *((string *) hd);
            route = tmp.first;
            docroot = tmp.second;
        }

        lua.add_servertableentry("docroot", docroot);
        if (uri.find(route) == 0) {
            uri = uri.substr(route.length());
            if (uri[0] != '/') uri = "/" + uri;
        }


        string infile = docroot + uri;


        if (access(infile.c_str(), R_OK) != 0) { // test, if file exists
            conn.status(Connection::NOT_FOUND);
            conn.header("Content-Type", "text/text; charset=utf-8");
            conn << "File not found\n";
            conn.flush();
            logger->error("FileHandler: \"") << infile << "\" not readable";
            return;
        }

        struct stat s;
        if (stat(infile.c_str(), &s) == 0) {
            if (!(s.st_mode & S_IFREG)) { // we have not a regular file, do nothing!
                return;
            }
        }
        else {
            return;
        }

        pair<string,string> mime = GetMimetype::getMimetype(infile);

        size_t extpos = uri.find_last_of('.');
        std::string extension;
        if (extpos != std::string::npos) {
            extension = uri.substr(extpos + 1);
        }
        try {
            if ((extension == "html") && (mime.first == "text/html")) {
                conn.header("Content-Type", "text/html; charset=utf-8");
                conn.sendFile(infile);
            }
            else if (extension == "js") {
                conn.header("Content-Type", "application/javascript; charset=utf-8");
                conn.sendFile(infile);
            }
            else if (extension == "css") {
                conn.header("Content-Type", "text/css; charset=utf-8");
                conn.sendFile(infile);
            }
            else if (extension == "lua") { // pure lua
                conn.setBuffer();
                ifstream inf;
                inf.open(infile);//open the input file

                stringstream sstr;
                sstr << inf.rdbuf();//read the file
                string luacode = sstr.str();//str holds the content of the file

                try {
                    if (lua.executeChunk(luacode) != 1) {
                        conn.flush();
                        return;
                    }
                }
                catch (Error &err) {
                    try {
                        conn.status(Connection::INTERNAL_SERVER_ERROR);
                        conn.header("Content-Type", "text/text; charset=utf-8");
                        conn << "Lua Error:\r\n==========\r\n" << err << "\r\n";
                        conn.flush();
                    }
                    catch(int i) {
                        return;
                    }
                    logger->error("FileHandler: error executing lua chunk: ") << err;
                    return;
                }
                conn.flush();
            }
            else if (extension == "elua") { // embedded lua <lua> .... </lua>
                conn.setBuffer();
                ifstream inf;
                inf.open(infile);//open the input file

                stringstream sstr;
                sstr << inf.rdbuf();//read the file
                string eluacode = sstr.str(); // eluacode holds the content of the file

                size_t pos = 0;
                size_t end = 0; // end of last lua code (including </lua>)
                while ((pos = eluacode.find("<lua>", end)) != string::npos) {
                    string htmlcode = eluacode.substr(end, pos - end);
                    pos += 5;

                    if (!htmlcode.empty()) conn << htmlcode; // send html...

                    string luastr;
                    if ((end = eluacode.find("</lua>", pos)) != string::npos) { // we found end;
                        luastr = eluacode.substr(pos, end - pos);
                        end += 6;
                    }
                    else {
                        luastr = eluacode.substr(pos);
                    }

                    try {
                        if (lua.executeChunk(luastr) != 1) {
                            conn.flush();
                            return;
                        }
                    }
                    catch (Error &err) {
                        try {
                            conn.status(Connection::INTERNAL_SERVER_ERROR);
                            conn.header("Content-Type", "text/text; charset=utf-8");
                            conn << "Lua Error:\r\n==========\r\n" << err << "\r\n";
                            conn.flush();
                        }
                        catch (int i) {
                            return;
                        }
                        logger->error("FileHandler: error executing lua chunk") << err;
                        return;
                    }
                }
                string htmlcode = eluacode.substr(end);
                conn << htmlcode;
                conn.flush();
            }
            else {
                conn.header("Content-Type", mime.first + "; " + mime.second);
                conn.sendFile(infile);
            }
        }
        catch (int i) {
            return; // we have an io error => just return, the thread will exit
        }
        catch (Error &err) {
            try {
                conn.status(Connection::INTERNAL_SERVER_ERROR);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << err;
                conn.flush();
            }
            catch (int i) {
                return;
            }
            logger->error("FileHandler: internal error: ") << err;
            return;
        }
    }
    //=========================================================================

    Server::Server(int port_p, unsigned nthreads_p, const std::string userid_str, const std::string &logfile_p, const std::string &loglevel_p)
        : port(port_p), _nthreads(nthreads_p), _logfilename(logfile_p), _loglevel(loglevel_p)
    {
        _ssl_port = -1;

        //
        // we use a semaphore object to control the number of threads
        //
        semname = "shttps";
        semname += to_string(port);
        _user_data = NULL;
        running = false;
        _keep_alive_timeout = 20;

        spdlog::set_async_mode(1048576);
        _logger = spdlog::rotating_logger_mt(loggername, _logfilename, 1048576 * 20, 3, true);

        if (_loglevel == "TRACE") {
            spdlog::set_level(spdlog::level::trace);
        }
        else if (_loglevel == "DEBUG") {
            spdlog::set_level(spdlog::level::debug);
        }
        else if (_loglevel == "INFO") {
            spdlog::set_level(spdlog::level::info);
        }
        else if (_loglevel == "NOTICE") {
            spdlog::set_level(spdlog::level::notice);
        }
        else if (_loglevel == "WARN") {
            spdlog::set_level(spdlog::level::warn);
        }
        else if (_loglevel == "ERROR") {
            spdlog::set_level(spdlog::level::err);
        }
        else if (_loglevel == "CRITICAL") {
            spdlog::set_level(spdlog::level::critical);
        }
        else if (_loglevel == "ALERT") {
            spdlog::set_level(spdlog::level::alert);
        }
        else if (_loglevel == "EMER") {
            spdlog::set_level(spdlog::level::emerg);
        }
        else if (_loglevel == "OFF") {
            spdlog::set_level(spdlog::level::off);
        }
        else {
            spdlog::set_level(spdlog::level::debug);
        }

        //
        // Her we check if we have to change to a different uid. This can only be done
        // if the server runs originally as root!
        //
        bool setuid_done = false;
        if (!userid_str.empty() && (getuid() == 0)) { // must be root to setuid() !!
            struct passwd pwd, *res;

            size_t buffer_len = sysconf(_SC_GETPW_R_SIZE_MAX) * sizeof(char);
            char *buffer = new char[buffer_len];
            getpwnam_r(userid_str.c_str(), &pwd, buffer, buffer_len, &res);
            if (res != NULL) {
                setuid(pwd.pw_uid);
                setgid(pwd.pw_gid);
                setuid_done = true;
            }
            else {
                cerr << "Couldn't setuid() to " << userid_str << "!" << endl;
                exit (-1);
            }
            delete [] buffer;
        }
        else {
            _logger->warn("Couldnt switch tu user ") << userid_str << "! You must start SIPI as root!";
        }

        if (setuid_done) {
            _logger->notice("Server will run as user ") << userid_str << " (" << getuid() << ")";
        }


#ifdef SHTTPS_ENABLE_SSL
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();
#endif

        ::sem_unlink(semname.c_str()); // unlink to be sure that we start from scratch
        _semaphore = ::sem_open(semname.c_str(), O_CREAT, 0x755, _nthreads);
        _semcnt = _nthreads;
    }
    //=========================================================================

#ifdef SHTTPS_ENABLE_SSL
    void Server::jwt_secret(const string &jwt_secret_p) {
        _jwt_secret = jwt_secret_p;
        int l;
        if ((l = _jwt_secret.size()) < 32) {
            for (int i = 0; i < (32 - l); i++) {
            _jwt_secret.push_back('A' + i);
            }
        }
    }
    //=========================================================================
#endif



    RequestHandler Server::getHandler(Connection &conn, void **handler_data_p)
    {
        map<std::string,RequestHandler>::reverse_iterator item;

        size_t max_match_len = 0;
        string matching_path;
        RequestHandler matching_handler = NULL;

        for (item = handler[conn.method()].rbegin(); item != handler[conn.method()].rend(); ++item) {
            size_t len = conn.uri().length() < item->first.length() ? conn.uri().length() : item->first.length();
            if (item->first == conn.uri().substr(0, len)) {
                if (len > max_match_len) {
                    max_match_len = len;
                    matching_path = item->first;
                    matching_handler = item->second;
                }
            }
        }
        if (max_match_len > 0) {
            *handler_data_p = handler_data[conn.method()][matching_path];
            return matching_handler;
        }
        return default_handler;
    }
    //=============================================================================


    static int prepare_socket(int port) {
        int sockfd;
        struct sockaddr_in serv_addr;

        auto logger = spdlog::get(loggername);

        sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            logger->emerg("Could not create socket: ") << strerror(errno);
            exit(1);
        }

        int optval = 1;
        if (::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) {
            logger->emerg("Could not set socket option: ") << strerror(errno);
            exit(1);
        }

        /* Initialize socket structure */
        bzero((char *) &serv_addr, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        /* Now bind the host address using bind() call.*/
        if (::bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            logger->emerg("Could not bind socket: ") << strerror(errno);
            exit(1);
        }

        if (::listen(sockfd, SOMAXCONN) < 0) {
            logger->emerg("Could not listen on socket: ") << strerror(errno);
            exit (1);
        }
        return sockfd;
    }
    //=========================================================================


    static void idle_add(pthread_t tid_p) {
        idlelock.lock();
        idle_thread_ids.push_back(tid_p);
        idlelock.unlock();
    }
    //=========================================================================

    static void idle_remove(pthread_t tid_p) {
        int index = 0;
        bool in_idle = false;
        idlelock.lock();
        for (auto tid : idle_thread_ids) {
            if (tid == tid_p) {
                in_idle = true;
                break;
            }
            index++;
        }
        if (in_idle) {
            //
            // the vector is in idle state, remove it from the idle vector
            //
            idle_thread_ids.erase(idle_thread_ids.begin() + index);
        }
        idlelock.unlock();
    }
    //=========================================================================

    static int close_socket(TData *tdata) {
        auto logger = spdlog::get(loggername);

#ifdef SHTTPS_ENABLE_SSL
        if (tdata->cSSL != NULL) {
            int sstat;
            while ((sstat = SSL_shutdown(tdata->cSSL)) == 0);
            if (sstat < 0) {
                logger->warn("SSL socket error: shutdown of socket failed! Reason: ") << SSL_get_error(tdata->cSSL, sstat);
            }
            SSL_free(tdata->cSSL);
            tdata->cSSL = NULL;
        }
#endif
        if (shutdown(tdata->sock, SHUT_RDWR) < 0) {
            logger->warn("Error shutdown socket! Reason: ") << strerror(errno);
        }
        if (close(tdata->sock) == -1) {
            logger->warn("Error closing socket! Reason: ") << strerror(errno);
        }

        return 0;
    }
    //=========================================================================


    static void *process_request(void *arg)
    {
        signal(SIGPIPE, SIG_IGN);

        TData *tdata = (TData *) arg;
        pthread_t my_tid = pthread_self();

        auto logger = spdlog::get(loggername);

        //
        // now we create the socket StreamSock
        //
        SockStream *sockstream;
#ifdef SHTTPS_ENABLE_SSL
        if (tdata->cSSL != NULL) {
            sockstream = new SockStream(tdata->cSSL);
        }
        else {
            sockstream = new SockStream(tdata->sock);
        }
#else
        sockstream = new SockStream(tdata->sock);
#endif
        istream ins(sockstream);
        ostream os(sockstream);

        ThreadStatus tstatus;
        int keep_alive = 1;
        do {
#ifdef SHTTPS_ENABLE_SSL
            if (tdata->cSSL != NULL) {
                tstatus = tdata->serv->processRequest(&ins, &os, tdata->peer_ip, tdata->peer_port, true, keep_alive);
            }
            else {
                tstatus = tdata->serv->processRequest(&ins, &os, tdata->peer_ip, tdata->peer_port, false, keep_alive);
            }
#else
            tstatus = tdata->serv->processRequest(&ins, &os, tdata->peer_ip, tdata->peer_port, false, keep_alive);
#endif

            if (tstatus == CLOSE) break; // it's CLOSE , let's get out of the loop

            //
            // tstatus is CONTINUE. Let's check if we got in the meantime a CLOSE message from the main...
            //
            pollfd readfds[2];
            readfds[0] = { tdata->commpipe_read, POLLIN, 0};
            readfds[1] = { tdata->sock, POLLIN, 0};
            if (poll(readfds, 2, 0) < 0) { // no blocking here!!!
                logger->error("Non-blocking poll failed: Line: ") << to_string(__LINE__);
                tstatus = CLOSE;
                break; // accept returned something strange – probably we want to shutdown the server
            }
            if (readfds[1].revents & POLLIN) {
                continue;
            }
            if (readfds[0].revents & POLLIN) { // something on the pipe from the main
                //
                // we got a message on the communication channel from the main thread...
                //
                if (Server::CommMsg::read(tdata->commpipe_read) != 0) {
                    keep_alive = -1;
                    break;
                }
                keep_alive = -1;
                if (readfds[1].revents & POLLIN) { // but we already have data...
                    continue; // continue loop
                }
                else {
                    break;
                }
            }

            //
            // we check if a new request is waiting for the semaphore which limits the number of threads
            //
            if (tdata->serv->semaphore_get() < 0) { //Another thread is waiting, give him a chance...
                keep_alive = -1;
                if (readfds[1].revents & POLLIN) { // but we already have data...
                    continue; // we have data, we will close after the next round...
                }
                else {
                    break;
                }
            }

            //
            // if we can continue and have a keep_alive, let's set the thread to idle...
            //
            idle_add(my_tid);

            //
            // use poll to wait...
            //
            readfds[0] = { tdata->commpipe_read, POLLIN, 0};
            readfds[1] = { tdata->sock, POLLIN, 0};
            if (poll(readfds, 2, keep_alive*1000) < 0) {
                logger->error("Blocking poll failed at line: ") << to_string(__LINE__) << " ERROR: " << strerror(errno);
                tstatus = CLOSE;
                idle_remove(my_tid);
                break; // accept returned something strange – probably we want to shutdown the server
            }
            if (!(readfds[0].revents | readfds[1].revents)) { // we got a timeout from poll
                //
                // timeout from poll
                //
                tstatus = CLOSE;
                idle_remove(my_tid);
                break;
            }
            if (readfds[0].revents & POLLIN) { // something on the pipe...
                //
                // we got a message on the communication channel from the main thread...
                //
                if (Server::CommMsg::read(tdata->commpipe_read) != 0) {
                    keep_alive = -1;
                    idle_remove(my_tid);
                    break;
                }
                keep_alive = -1;
                idle_remove(my_tid);
                if (readfds[1].revents & POLLIN) { // but we already have data...
                    continue; // continue loop
                }
                else {
                    break;
                }
            }
        } while (tstatus == CONTINUE);

        //
        // let's close the socket
        //
        close_socket(tdata);

        delete sockstream;

        if (close(tdata->commpipe_read) == -1) {
            logger->error("Commpipe_read close error: ") << string(strerror(errno));
        }
        int compipe_write = tdata->serv->get_thread_pipe(pthread_self());
        if (compipe_write > 0) {
            if (close (compipe_write) == -1) {
                logger->error("Commpipe_write close error: ") << string(strerror(errno));
            }
        }
        else {
            logger->debug("Thread to stop does no longer exist...!");
        }
        tdata->serv->remove_thread(pthread_self());
        tdata->serv->semaphore_leave();

        delete tdata;

        return NULL;
    }
    //=========================================================================


    void Server::run()
    {
        _logger->info("Starting shttps server... with ") << to_string(_nthreads) << " threads";

        //
        // now we are adding the lua routes
        //
        for (auto & route : _lua_routes) {
            route.script = _scriptdir + "/" + route.script;
            addRoute(route.method, route.route, ScriptHandler, &(route.script));
            _logger->info("Added route '") << route.route << "' with script '" << route.script << "'";
        }
        _sockfd = prepare_socket(port);
        _logger->notice("Server listening on port ") << to_string(port);
        if (_ssl_port > 0) {
            _ssl_sockfd = prepare_socket(_ssl_port);
            _logger->notice("Server listening on SSL port ") << to_string(_ssl_port);
        }

        pipe(stoppipe); // ToDo: Errorcheck
        pthread_t thread_id;
        running = true;
        int count = 0;
        while(running) {
            int sock;

            pollfd readfds[3];
            int n_readfds = 0;
            readfds[0] = { _sockfd, POLLIN, 0}; n_readfds++;
            readfds[1] = {stoppipe[0], POLLIN, 0}; n_readfds++;
            if (_ssl_port > 0) {
                readfds[2] = {_ssl_sockfd, POLLIN, 0}; n_readfds++;
            }

            if (poll(readfds, n_readfds, -1) < 0) {
                _logger->error("Blocking poll failed at line: ") << to_string(__LINE__) << " ERROR: " << strerror(errno);
                running = false;
                break;
            }
            count++;
            if (readfds[0].revents & POLLIN) {
                sock = _sockfd;
            }
            else if (readfds[1].revents & POLLIN) {
                sock = stoppipe[0];
                char buf[2];
                read(stoppipe[0], buf, 1);
                running = false;
                break;
            }
            else if ((_ssl_port > 0) && (readfds[2].revents & POLLIN)) {
                sock = _ssl_sockfd;
            }
            else {
                _logger->error("Blocking poll failed at line: ") << to_string(__LINE__) << " ERROR: Strange!!";
                running = false;
                break; // accept returned something strange – probably we want to shutdown the server
            }
            struct sockaddr_storage cli_addr;
            socklen_t cli_size = sizeof(cli_addr);
            int newsockfs = ::accept(sock, (struct sockaddr *) &cli_addr, &cli_size);

            if (newsockfs <= 0) {
                _logger->error("::accept error! ERROR: ") << strerror(errno);
                break; // accept returned something strange – probably we want to shutdown the server
            }

            //
            // get peer address
            //
            char client_ip[INET6_ADDRSTRLEN];
            int peer_port;

            if (cli_addr.ss_family == AF_INET) {
                struct sockaddr_in *s = (struct sockaddr_in *) &cli_addr;
                peer_port = ntohs(s->sin_port);
                inet_ntop(AF_INET, &s->sin_addr, client_ip, sizeof client_ip);
            } else if (cli_addr.ss_family == AF_INET6) { // AF_INET6
                struct sockaddr_in6 *s = (struct sockaddr_in6 *) &cli_addr;
                peer_port = ntohs(s->sin6_port);
                inet_ntop(AF_INET6, &s->sin6_addr, client_ip, sizeof client_ip);
            }
            _logger->notice("Accepted connection from: ") << client_ip;

            TData *tmp = new TData;
            tmp->sock = newsockfs;
            tmp->peer_ip = client_ip;
            tmp->peer_port = peer_port;
            tmp->serv = this;

#ifdef SHTTPS_ENABLE_SSL
            SSL *cSSL = NULL;
            if (sock == _ssl_sockfd) {
                SSL_CTX *sslctx;
                try {
                    if ((sslctx = SSL_CTX_new(SSLv23_server_method())) == NULL) {
                        _logger->error("OpenSSL error: 'SSL_CTX_new()' failed!");
                        throw SSLError(__file__, __LINE__, "OpenSSL error: 'SSL_CTX_new()' failed!");
                    }
                    SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);
                    if (SSL_CTX_use_certificate_file(sslctx, _ssl_certificate.c_str(), SSL_FILETYPE_PEM) != 1) {
                        string msg = "OpenSSL error: 'SSL_CTX_use_certificate_file(\"" + _ssl_certificate + "\")' failed!";
                        throw SSLError(__file__, __LINE__, msg);
                    }
                    if (SSL_CTX_use_PrivateKey_file(sslctx, _ssl_key.c_str(), SSL_FILETYPE_PEM) != 1) {
                        string msg = "OpenSSL error: 'SSL_CTX_use_PrivateKey_file(\"" + _ssl_certificate + "\")' failed!";
                        throw SSLError(__file__, __LINE__, msg);
                    }
                    if (!SSL_CTX_check_private_key(sslctx)) {
                        throw SSLError(__file__, __LINE__, "OpenSSL error: 'SSL_CTX_check_private_key()' failed!");
                    }
                    if ((cSSL = SSL_new(sslctx)) == NULL) {
                        throw SSLError(__file__, __LINE__, "OpenSSL error: 'SSL_new()' failed!");
                    }
                    if (SSL_set_fd(cSSL, newsockfs) != 1) {
                        throw SSLError(__file__, __LINE__, "OpenSSL error: 'SSL_set_fd()' failed!");
                    }

                    //Here is the SSL Accept portion.  Now all reads and writes must use SS
                    int suc;
                    if ((suc = SSL_accept(cSSL)) <= 0) {
                        throw SSLError(__file__, __LINE__, "OpenSSL error: 'SSL_accept()' failed!");
                    }
               }
                catch (SSLError &err) {
                    _logger->error(err.to_string());
                    int sstat;
                    while ((sstat = SSL_shutdown(cSSL)) == 0);
                    if (sstat < 0) {
                        _logger->warn("SSL socket error: shutdown (2) of socket failed! Reason: ") << SSL_get_error(cSSL, sstat);
                    }
                    SSL_free(cSSL);
                    cSSL = NULL;
                }
            }
            tmp->cSSL = cSSL;
#endif

            if (semaphore_get() <= 0) {
                //
                // we would be blocked by the semaphore... Get an idle thread...
                //
                idlelock.lock();
                if (idle_thread_ids.size() > 0) {
                    pthread_t tid = idle_thread_ids.front();
                    idlelock.unlock();
                    int pipe_id = get_thread_pipe(tid);
                    if (pipe_id > 0) {
                        Server::CommMsg::send(pipe_id);
                    }
                    else {
                        _logger->debug("Thread to stop does no longer exist...!");
                    }
                }
                else {
                    idlelock.unlock();
                }
            }
            semaphore_wait();
            int commpipe[2];

            if (socketpair(PF_LOCAL, SOCK_STREAM, 0, commpipe) != 0) {
                _logger->error("Creating pipe failed! ERROR: ") << strerror(errno);
                running = false;
                break;
            }

            tmp->commpipe_read = commpipe[1]; // read end;

            //
            // we create detached threads because we will not be able to use pthread_join()
            //
            pthread_attr_t tattr;
            pthread_attr_init(&tattr);
            pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
            //int stacksize = (PTHREAD_STACK_MIN + 1024*1024*3);
            //pthread_attr_setstacksize(&tattr, stacksize);

            if( pthread_create( &thread_id, &tattr,  process_request, (void *) tmp) < 0) {
                _logger->error("Could not create thread") << strerror(errno);
                running = false;
                break;
            }
#ifdef SHTTPS_ENABLE_SSL
            if (tmp->cSSL != NULL) {
                add_thread(thread_id, commpipe[0], tmp->sock, tmp->cSSL);
            }
            else {
                add_thread(thread_id, commpipe[0], tmp->sock);
            }
#else
            add_thread(thread_id, commpipe[0], tmp->sock);
#endif
        }
        _logger->notice("Server shutting down!");


        //
        // let's send the close message to all running threads
        //
        int i = 0;
        threadlock.lock();
        int num_active_threads = thread_ids.size();
        pthread_t *ptid = new pthread_t[num_active_threads];
        for(auto const &tid : thread_ids) {
            ptid[i++] = tid.first;
            Server::CommMsg::send(tid.second.commpipe_write);
        }
        threadlock.unlock();

        close(stoppipe[0]);
        close(stoppipe[1]);

        //
        // we have closed all sockets, now we can wait for the threads to terminate
        //
        for (int i = 0; i < num_active_threads; i++) {
            pthread_join(ptid[i], NULL);
        }

        delete [] ptid;
    }
    //=========================================================================


    void Server::addRoute(Connection::HttpMethod method_p, const string &path_p, RequestHandler handler_p, void *handler_data_p)
    {
        handler[method_p][path_p] = handler_p;
        handler_data[method_p][path_p] = handler_data_p;
    }
    //=========================================================================


    ThreadStatus Server::processRequest(istream *ins, ostream *os, string &peer_ip, int peer_port, bool secure, int &keep_alive)
    {
        if (_tmpdir.empty()) {
            throw Error(__file__, __LINE__, "_tmpdir is empty");
        }

        if (ins->eof() || os->eof()) return CLOSE;

        try {
            Connection conn(this, ins, os, _tmpdir);

            if (keep_alive <= 0) {
                conn.keepAlive(false);
            }
            keep_alive = conn.setupKeepAlive(_keep_alive_timeout);

            conn.peer_ip(peer_ip);
            conn.peer_port(peer_port);
            conn.secure(secure);

            if (conn.resetConnection()) {
                if (conn.keepAlive()) {
                    return CONTINUE;
                }
                else {
                    return CLOSE;
                }
            }

            //
            // Setting up the Lua server
            //
            LuaServer luaserver(_initscript, true);
            luaserver.createGlobals(conn);
            for (auto &global_func : lua_globals) {
                global_func.func(luaserver.lua(), conn, global_func.func_dataptr);
            }

            void *hd = NULL;
            try {
                RequestHandler handler = getHandler(conn, &hd);
                handler(conn, luaserver, _user_data, hd);
            }
            catch (int i) {
                _logger->debug("Possibly socket closed by peer!");
                return CLOSE; // or CLOSE ??
            }
            if (!conn.cleanupUploads()) {
                _logger->error("Cleanup of uploaded files failed!");
            }
            if (conn.keepAlive()) {
                return CONTINUE;
            }
            else {
                return CLOSE;
            }
        }
        catch (int i) { // "error" is thrown, if the socket was closed from the main thread...
            _logger->debug("Socket connection: timeout or socket closed from main");
            return CLOSE;
        }
        catch(Error &err) {
            try {
                _logger->error("Internal server error: ") << err;
                *os << "HTTP/1.1 500 INTERNAL_SERVER_ERROR\r\n";
                *os << "Content-Type: text/plain\r\n";
                stringstream ss;
                ss << err;
                *os << "Content-Length: " << ss.str().length() << "\r\n\r\n";
                *os << ss.str();
            }
            catch (int i) {
                _logger->error("Possibly socket closed by peer!");
            }
            return CLOSE;
        }
    }
    //=========================================================================

    void Server::add_thread(pthread_t thread_id_p, int commpipe_write_p, int sock_id) {
#ifdef SHTTPS_ENABLE_SSL
        GenericSockId sid = {sock_id, NULL, commpipe_write_p};
#else
        GenericSockId sid = {sock_id, commpipe_write_p};
#endif
        threadlock.lock();
        thread_ids[thread_id_p] = sid;
        threadlock.unlock();
    }
    //=========================================================================

#ifdef SHTTPS_ENABLE_SSL
    void Server::add_thread(pthread_t thread_id_p, int commpipe_write_p, int sock_id, SSL *cSSL) {
        GenericSockId sid = {sock_id, cSSL, commpipe_write_p};
        threadlock.lock();
        thread_ids[thread_id_p] = sid;
        threadlock.unlock();
    }
#endif
    //=========================================================================

    int Server::get_thread_sock(pthread_t thread_id_p) {
        GenericSockId sid;
        threadlock.lock();
        try {
            sid = thread_ids.at(thread_id_p);
        }
        catch (const std::out_of_range& oor) {
            threadlock.unlock();
            return -1;
        }
        threadlock.unlock();
        return sid.sid;
    }
    //=========================================================================

    int Server::get_thread_pipe(pthread_t thread_id_p) {
        GenericSockId sid;
        threadlock.lock();
        try {
            sid = thread_ids.at(thread_id_p);
        }
        catch (const std::out_of_range& oor) {
            threadlock.unlock();
            return -1;
        }
        threadlock.unlock();
        return sid.commpipe_write;
    }
    //=========================================================================


#ifdef SHTTPS_ENABLE_SSL
     SSL *Server::get_thread_ssl(pthread_t thread_id_p) {
        GenericSockId sid;
        threadlock.lock();
        try {
            sid = thread_ids.at(thread_id_p);
        }
        catch (const std::out_of_range& oor) {
            threadlock.unlock();
            return NULL;
        }
        threadlock.unlock();
        return sid.ssl_sid;
    }
    //=========================================================================
#endif


    void Server::remove_thread(pthread_t thread_id_p) {
        int index = 0;
        bool in_idle = false;
        idlelock.lock();
        for (auto tid : idle_thread_ids) {
            if (tid == thread_id_p) {
                in_idle = true;
                break;
            }
            index++;
        }
        if (in_idle) {
            //
            // the vector is in idle state, remove it from the idle vector
            //
            idle_thread_ids.erase(idle_thread_ids.begin() + index);
        }
        idlelock.unlock();
        threadlock.lock();
        thread_ids.erase(thread_id_p);
        threadlock.unlock();
    }
    //=========================================================================

    void Server::debugmsg(const std::string &msg) {
        debugio.lock();
        std::cerr << msg << std::endl;
        debugio.unlock();
    }
    //=========================================================================

}
