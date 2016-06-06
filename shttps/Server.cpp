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

#include <netinet/in.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <pthread.h>

#include "Global.h"
#include "SockStream.h"
#include "Server.h"
#include "LuaServer.h"
#include "GetMimetype.h"

static const char __file__[] = __FILE__;

using namespace std;

namespace shttps {

    const char loggername[] = "shttps-logger"; // see Global.h !!

    typedef struct {
        int sock;
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
                    lua.executeChunk(luacode);
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

                    conn << htmlcode; // send html...

                    string luastr;
                    if ((end = eluacode.find("</lua>", pos)) != string::npos) { // we found end;
                        luastr = eluacode.substr(pos, end - pos);
                        end += 6;
                    }
                    else {
                        luastr = eluacode.substr(pos);
                    }

                    try {
                        lua.executeChunk(luastr);
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
            size_t start_at = 0;
            size_t end_at = 0;
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
                    lua.executeChunk(luacode);
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

                    conn << htmlcode; // send html...

                    string luastr;
                    if ((end = eluacode.find("</lua>", pos)) != string::npos) { // we found end;
                        luastr = eluacode.substr(pos, end - pos);
                        end += 6;
                    }
                    else {
                        luastr = eluacode.substr(pos);
                    }

                    try {
                        lua.executeChunk(luastr);
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

    Server::Server(int port_p, unsigned nthreads_p, const std::string &logfile_p)
        : port(port_p), _nthreads(nthreads_p)
    {
        semname = "shttps";
        semname += to_string(port);
        _user_data = NULL;
        running = false;
        _keep_alive_timeout = 20;
        spdlog::set_async_mode(1048576);
        _logger = spdlog::rotating_logger_mt(loggername, logfile_p, 1048576 * 5, 3, true);

        spdlog::set_level(spdlog::level::debug);
    }
    //=========================================================================


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

    static void *process_request(void *arg)
    {
        TData *tdata = (TData *) arg;
        tdata->serv->processRequest(tdata->sock);
        tdata->serv->remove_thread(pthread_self());
        ::sem_post(tdata->serv->semaphore());
        free(tdata);
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

        struct sockaddr_in serv_addr, cli_addr;

        ::sem_unlink(semname.c_str()); // unlink to be sure that we start from scratch
        _semaphore = ::sem_open(semname.c_str(), O_CREAT, 0x755, _nthreads);

        socklen_t cli_size = sizeof(cli_addr);

        _sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (_sockfd < 0) {
            perror("ERROR on socket");
            _logger->error("Could not create socket: ") << strerror(errno);
            exit(1);
        }

        int optval = 1;
        if (::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) {
            perror("ERROR on setsocketopt");
            _logger->error("Could not set socket option: ") << strerror(errno);
            exit(1);
        }


        /* Initialize socket structure */
        bzero((char *) &serv_addr, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        /* Now bind the host address using bind() call.*/
        if (::bind(_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("ERROR on binding");
            _logger->error("Could not bind socket: ") << strerror(errno);
            exit(1);
        }

        if (::listen(_sockfd, SOMAXCONN) < 0) {
            perror("ERROR on listen");
            _logger->error("Could not listen on socket: ") << strerror(errno);
            exit (1);
        }

        _logger->info("Server listening on port ") << to_string(port);

        pthread_t thread_id;
        running = true;
        while(running) {
            int newsockfs = ::accept(_sockfd, (struct sockaddr *) &cli_addr, &cli_size);
            if (newsockfs <= 0) {
                _logger->debug("accept returned ") << to_string(newsockfs) << ". Shutdown?";
                break; // accept returned something strange – probably we want to shutdown the server
            }

            //
            // get peer address
            //
            struct sockaddr_in peer_addr;
            socklen_t peer_addr_size = sizeof(struct sockaddr_in);
            int res = getpeername(newsockfs, (struct sockaddr *)&peer_addr, &peer_addr_size);
            char client_ip[20];
            strcpy(client_ip, inet_ntoa(peer_addr.sin_addr));
            _logger->info("Accepted connection from: ") << client_ip;

            TData *tmp = (TData *) malloc(sizeof(TData));
            tmp->sock = newsockfs;
            tmp->serv = this;
            ::sem_wait(_semaphore) ;
            if( pthread_create( &thread_id, NULL,  process_request , (void *) tmp) < 0) {
                perror("could not create thread");
                _logger->error("Could not create thread") << strerror(errno);
                exit (1);
            }
            add_thread(thread_id, newsockfs);
        }
        _logger->debug("Server shutting down!");

        //
        // now we make all threads to terminate by closing their sockets
        //
        int num_active_threads = thread_ids.size();
        pthread_t *ptid = new pthread_t[num_active_threads];
        int *sid = new int[num_active_threads];
        int i = 0;
        for(auto const &tid : thread_ids) {
            ptid[i] = tid.first;
            sid[i] = tid.second;
            i++;
        }
        for (int i = 0; i < num_active_threads; i++) {
            threadlock.lock();
            close(sid[i]);
            //
            // we indicate to the thread that the socked has been closed (and it's not a timeout)
            // by setting the socked_id in the thread_ids-table to -1. This leads the thread
            // not to close it again. In case of a timeout the socket is being closed
            // by the thread.
            //
            thread_ids[ptid[i]] = -1;
            threadlock.unlock();
        }
        //
        // we have closed all sockets, now we can wait for the threads to terminate
        //
        for (int i = 0; i < num_active_threads; i++) {
            pthread_join(ptid[i], NULL);
        }
        delete [] ptid;
        delete [] sid;
    }
    //=========================================================================


    void Server::addRoute(Connection::HttpMethod method_p, const string &path_p, RequestHandler handler_p, void *handler_data_p)
    {
        handler[method_p][path_p] = handler_p;
        handler_data[method_p][path_p] = handler_data_p;
    }
    //=========================================================================


    void Server::processRequest(int sock)
    {
        int n;
        SockStream sockstream(sock);
        istream ins(&sockstream);
        ostream os(&sockstream);


        bool do_close = true;
        while (!ins.eof() && !os.eof()) {
            LuaServer luaserver(_initscript);
            Connection conn;
            try {
                if (_tmpdir.empty()) {
                    throw Error(__file__, __LINE__, "_tmpdir is empty");
                }
                conn = Connection(this, &ins, &os, _tmpdir);
            }
            catch (int i) { // "error" is thrown, if the socket was closed from the main thread...
                _logger->debug("Socket connection: timeout or socket closed from main");
                if (thread_ids[pthread_self()] == -1) {
                    _logger->debug("Socket is closed – no not close anymore");
                    do_close = false;
                }
                break;
            }
            catch(Error &err) {
                try {
                    os << "HTTP/1.1 500 INTERNAL_SERVER_ERROR\r\n";
                    os << "Content-Type: text/plain\r\n";
                    stringstream ss;
                    ss << err;
                    os << "Content-Length: " << ss.str().length() << "\r\n\r\n";
                    os << ss.str();
                    _logger->error("Internal server error!");
                }
                catch (int i) {
                    _logger->error("Possibly socket closed by peer!");
                }
                break;
            }

            conn.setupKeepAlive(sock, _keep_alive_timeout);

            if (conn.resetConnection()) {
                if (conn.keepAlive()) {
                    continue;
                }
                else {
                    break;
                }
            }

            luaserver.createGlobals(conn);
            for (auto & global_func : lua_globals) {
                global_func.func(luaserver.lua(), conn, global_func.func_dataptr);
            }

            void *hd = NULL;
            try {
                RequestHandler handler = getHandler(conn, &hd);
                _logger->debug("Calling user-supplied handler");
                handler(conn, luaserver, _user_data, hd);
            }
            catch (int i) {
                _logger->error("Possibly socket closed by peer!");
                break;
            }
            catch(Error &err) {
                try {
                    os << "HTTP/1.1 500 INTERNAL_SERVER_ERROR\r\n\r\n";
                    os << "Content-Type: text/plain\r\n";
                    stringstream ss;
                    ss << err;
                    os << "Content-Length: " << ss.str().length() << "\r\n\r\n";
                    os << ss.str();
                }
                catch (int i) {
                    _logger->error("Possibly socket closed by peer!");
                    break;
                }
                _logger->error("Internal server error! ") << err.to_string();
                break;
            }
            if (!conn.cleanupUploads()) {
                _logger->error("Cleanup of uploaded files failed!");
            }

            if (!conn.keepAlive()) break;

        }

        if (do_close) {
            _logger->debug("Socket was not closed, closing...");
            close (sock);
        }
    }
    //=========================================================================

}
