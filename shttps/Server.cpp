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
#include "Logger.h"
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

    const char loggername[] = "SIPI"; // see Global.h !!

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
        auto logger = Logger::getLogger(loggername);
        *logger << Logger::LogLevel::WARNING << "No handler available! Host: " << conn.host() << " Uri: " << conn.uri() << Logger::LogAction::FLUSH;

        return;
    }
    //=========================================================================


    void ScriptHandler(shttps::Connection &conn, LuaServer &lua, void *user_data, void *hd)
    {
        std::vector<std::string> headers = conn.header();
        std::string uri = conn.uri();

        auto logger = Logger::getLogger(loggername);

        string script = *((string *) hd);

        if (access(script.c_str(), R_OK) != 0) { // test, if file exists
            conn.status(Connection::NOT_FOUND);
            conn.header("Content-Type", "text/text; charset=utf-8");
            conn << "File not found\n";
            conn.flush();
            *logger << Logger::LogLevel::ERROR  << "ScriptHandler: \"" << script << "\" not readable!" << Logger::LogAction::FLUSH;
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
                inf.open(script); //open the input file

                stringstream sstr;
                sstr << inf.rdbuf(); //read the file
                string luacode = sstr.str();//str holds the content of the file

                try {
                    if (lua.executeChunk(luacode) < 0) {
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
                    *logger << Logger::LogLevel::ERROR  << "ScriptHandler: error executing lua script:  \"" << err << "\"" << Logger::LogAction::FLUSH;
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
                        if (lua.executeChunk(luastr) < 0) {
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
                        *logger << Logger::LogLevel::ERROR  << "ScriptHandler: error executing lua chunk:  \"" << err << "\""<< Logger::LogAction::FLUSH;
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
                *logger << Logger::LogLevel::ERROR  << "ScriptHandler: error executing script, unknown extension:  \"" << extension << "\"" << Logger::LogAction::FLUSH;
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
            *logger << Logger::LogLevel::ERROR  << "FileHandler: internal error:  \"" << err << "\"" << Logger::LogAction::FLUSH;
            return;
        }
    }
    //=========================================================================


    void FileHandler(shttps::Connection &conn, LuaServer &lua, void *user_data, void *hd)
    {
        std::vector<std::string> headers = conn.header();
        std::string uri = conn.uri();

        auto logger = Logger::getLogger(loggername);

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
            *logger << Logger::LogLevel::ERROR  << "FileHandler: \"" << infile << "\" not readable" << Logger::LogAction::FLUSH;
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
                    if (lua.executeChunk(luacode) < 0) {
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
                        *logger << Logger::LogLevel::ERROR  << "FileHandler: error executing lua chunk!" << Logger::LogAction::FLUSH;
                        return;
                    }
                    *logger << Logger::LogLevel::ERROR  << "FileHandler: error executing lua chunk: " << err << Logger::LogAction::FLUSH;
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
                        if (lua.executeChunk(luastr) < 0) {
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
                            *logger << Logger::LogLevel::ERROR  << "FileHandler: error executing lua chunk!" << Logger::LogAction::FLUSH;
                            return;
                        }
                        *logger << Logger::LogLevel::ERROR  << "FileHandler: error executing lua chunk: " << err << Logger::LogAction::FLUSH;
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
                *logger << Logger::LogLevel::ERROR  << "FileHandler: error executing lua chunk!" << Logger::LogAction::FLUSH;
                return;
            }
            *logger << Logger::LogLevel::ERROR  << "FileHandler: internal error: " << err << Logger::LogAction::FLUSH;
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

        Logger::LogLevel ll;
        if (_loglevel == "DEBUG") {
            ll = Logger::LogLevel::DEBUG;
        }
        else if (_loglevel == "INFO") {
            ll = Logger::LogLevel::INFORMATIONAL;
        }
        else if (_loglevel == "NOTICE") {
            ll = Logger::LogLevel::NOTICE;
        }
        else if (_loglevel == "WARN") {
            ll = Logger::LogLevel::WARNING;
        }
        else if (_loglevel == "ERROR") {
            ll = Logger::LogLevel::ERROR;
        }
        else if (_loglevel == "CRITICAL") {
            ll = Logger::LogLevel::DEBUG;
        }
        else if (_loglevel == "ALERT") {
            ll = Logger::LogLevel::ALERT;
        }
        else if (_loglevel == "EMER") {
            ll = Logger::LogLevel::EMERGENCY;
        }
        else {
            ll = Logger::LogLevel::ERROR;
        }

        auto logger = Logger::createLogger(loggername, _logfilename, ll);


        //
        // Her we check if we have to change to a different uid. This can only be done
        // if the server runs originally as root!
        //
        if (!userid_str.empty()) {
            if (getuid() == 0) { // must be root to setuid() !!
                struct passwd pwd, *res;

                size_t buffer_len = sysconf(_SC_GETPW_R_SIZE_MAX) * sizeof(char);
                char *buffer = new char[buffer_len];
                getpwnam_r(userid_str.c_str(), &pwd, buffer, buffer_len, &res);
                if (res != NULL) {
                    if (setuid(pwd.pw_uid) == 0) {
                        *logger << Logger::LogLevel::INFORMATIONAL << "Server will run as user \"" << userid_str << "\" (" << getuid() << ")" << Logger::LogAction::FLUSH;
                        if (setgid(pwd.pw_gid) == 0) {
                            *logger << Logger::LogLevel::INFORMATIONAL << "Server will run with group-id "  << getuid() << Logger::LogAction::FLUSH;
                        }
                        else {
                            *logger << Logger::LogLevel::ERROR << "setgid() failed! Reason: " << strerror(errno) << Logger::LogAction::FLUSH;
                        }
                    }
                    else {
                        *logger << Logger::LogLevel::ERROR << "setgid() failed! Reason: " << strerror(errno) << Logger::LogAction::FLUSH;
                    }
                }
                else {
                    *logger << Logger::LogLevel::ERROR << "Could not get uid of user \"" << userid_str << "\"! You must start SIPI as root!" << Logger::LogAction::FLUSH;
                }
                delete [] buffer;
            }
            else {
                *logger << Logger::LogLevel::ERROR << "Could not get uid of user \"" << userid_str << "\"! You must start SIPI as root!" << Logger::LogAction::FLUSH;
            }
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

        auto logger = Logger::getLogger(loggername);

        sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            *logger << Logger::LogLevel::ERROR << "Could not create socket: " << strerror(errno) << Logger::LogAction::FLUSH;
            exit(1);
        }

        int optval = 1;
        if (::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) {
            *logger << Logger::LogLevel::ERROR << "Could not set socket option: " << strerror(errno) << Logger::LogAction::FLUSH;
            exit(1);
        }

        /* Initialize socket structure */
        bzero((char *) &serv_addr, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        /* Now bind the host address using bind() call.*/
        if (::bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            *logger << Logger::LogLevel::ERROR << "Could not bind socket: " << strerror(errno) << Logger::LogAction::FLUSH;
            exit(1);
        }

        if (::listen(sockfd, SOMAXCONN) < 0) {
            *logger << Logger::LogLevel::ERROR << "Could not listen on socket: " << strerror(errno) << Logger::LogAction::FLUSH;
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
        auto logger = Logger::getLogger(loggername);

#ifdef SHTTPS_ENABLE_SSL
        if (tdata->cSSL != NULL) {
            int sstat;
            while ((sstat = SSL_shutdown(tdata->cSSL)) == 0);
            if (sstat < 0) {
                *logger << Logger::LogLevel::WARNING << "SSL socket error: shutdown of socket failed! Reason: " << SSL_get_error(tdata->cSSL, sstat) << Logger::LogAction::FLUSH;
            }
            SSL_free(tdata->cSSL);
            tdata->cSSL = NULL;
        }
#endif
        if (shutdown(tdata->sock, SHUT_RDWR) < 0) {
            *logger << Logger::LogLevel::WARNING << "Error shutdown socket! Reason: " << strerror(errno) << Logger::LogAction::FLUSH;
        }
        if (close(tdata->sock) == -1) {
            *logger << Logger::LogLevel::WARNING << "Error closing socket! Reason: " << strerror(errno) << Logger::LogAction::FLUSH;
        }

        return 0;
    }
    //=========================================================================


    static void *process_request(void *arg)
    {
        signal(SIGPIPE, SIG_IGN);

        TData *tdata = (TData *) arg;
        pthread_t my_tid = pthread_self();

        auto logger = Logger::getLogger(loggername);

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
                *logger << Logger::LogLevel::ERROR << "Non-blocking poll failed: Line: " << __LINE__ << Logger::LogAction::FLUSH;
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
                *logger << Logger::LogLevel::ERROR << "Blocking poll failed: Line: " << __LINE__ << Logger::LogAction::FLUSH;
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
            *logger << Logger::LogLevel::ERROR << "Commpipe_read close error: " << strerror(errno) << Logger::LogAction::FLUSH;
        }
        int compipe_write = tdata->serv->get_thread_pipe(pthread_self());
        if (compipe_write > 0) {
            if (close (compipe_write) == -1) {
                *logger << Logger::LogLevel::ERROR << "Commpipe_write close error: " << strerror(errno) << Logger::LogAction::FLUSH;
            }
        }
        else {
            *logger << Logger::LogLevel::DEBUG << "Thread to stop does no longer exist...!" << Logger::LogAction::FLUSH;
        }
        tdata->serv->remove_thread(pthread_self());
        tdata->serv->semaphore_leave();

        delete tdata;

        return NULL;
    }
    //=========================================================================


    void Server::run()
    {
        auto logger = Logger::getLogger(loggername);

        *logger << Logger::LogLevel::INFORMATIONAL << "Starting shttps server... with " <<_nthreads << " threads" << Logger::LogAction::FLUSH;

        //
        // now we are adding the lua routes
        //
        for (auto & route : _lua_routes) {
            route.script = _scriptdir + "/" + route.script;
            addRoute(route.method, route.route, ScriptHandler, &(route.script));
            *logger << Logger::LogLevel::INFORMATIONAL << "Added route " << route.route << " with script \""<< route.script<< "\"" << Logger::LogAction::FLUSH;
        }
        _sockfd = prepare_socket(port);
        *logger << Logger::LogLevel::INFORMATIONAL << "Server listening on port " << port << Logger::LogAction::FLUSH;
        if (_ssl_port > 0) {
            _ssl_sockfd = prepare_socket(_ssl_port);
            *logger << Logger::LogLevel::INFORMATIONAL << "Server listening on SSL port " << _ssl_port << Logger::LogAction::FLUSH;
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
                *logger << Logger::LogLevel::ERROR << "Blocking poll failed at line: " << __LINE__ <<" ERROR: " << strerror(errno) << Logger::LogAction::FLUSH;
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
                *logger << Logger::LogLevel::ERROR << "Blocking poll failed at line: " << __LINE__ << " ERROR: strange thing happened!" << Logger::LogAction::FLUSH;
                running = false;
                break; // accept returned something strange – probably we want to shutdown the server
            }
            struct sockaddr_storage cli_addr;
            socklen_t cli_size = sizeof(cli_addr);
            int newsockfs = ::accept(sock, (struct sockaddr *) &cli_addr, &cli_size);

            if (newsockfs <= 0) {
                *logger << Logger::LogLevel::ERROR << "accept error! ERROR: " << strerror(errno) << Logger::LogAction::FLUSH;
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
            }
            else if (cli_addr.ss_family == AF_INET6) { // AF_INET6
                struct sockaddr_in6 *s = (struct sockaddr_in6 *) &cli_addr;
                peer_port = ntohs(s->sin6_port);
                inet_ntop(AF_INET6, &s->sin6_addr, client_ip, sizeof client_ip);
            }
            else {
                peer_port = -1;
            }
            *logger << Logger::LogLevel::INFORMATIONAL << Logger::LogAction::FORCE << "Accepted connection from: " << client_ip << Logger::LogAction::FLUSH;

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
                        *logger << Logger::LogLevel::INFORMATIONAL << "OpenSSL error: 'SSL_CTX_new()' failed!" << Logger::LogAction::FLUSH;
                        throw SSLError(__file__, __LINE__, "OpenSSL error: 'SSL_CTX_new()' failed!");
                    }
                    SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);
                    if (SSL_CTX_use_certificate_file(sslctx, _ssl_certificate.c_str(), SSL_FILETYPE_PEM) != 1) {
                        string msg = "OpenSSL error: 'SSL_CTX_use_certificate_file(\"" + _ssl_certificate + "\")' failed!";
                        *logger << Logger::LogLevel::ERROR << msg << Logger::LogAction::FLUSH;
                        throw SSLError(__file__, __LINE__, msg);
                    }
                    if (SSL_CTX_use_PrivateKey_file(sslctx, _ssl_key.c_str(), SSL_FILETYPE_PEM) != 1) {
                        string msg = "OpenSSL error: 'SSL_CTX_use_PrivateKey_file(\"" + _ssl_certificate + "\")' failed!";
                        *logger << Logger::LogLevel::ERROR << msg << Logger::LogAction::FLUSH;
                        throw SSLError(__file__, __LINE__, msg);
                    }
                    if (!SSL_CTX_check_private_key(sslctx)) {
                        string msg = "OpenSSL error: 'SSL_CTX_check_private_key()' failed!";
                        *logger << Logger::LogLevel::ERROR << msg << Logger::LogAction::FLUSH;
                        throw SSLError(__file__, __LINE__, msg);
                    }
                    if ((cSSL = SSL_new(sslctx)) == NULL) {
                        string msg = "OpenSSL error: 'SSL_new()' failed!";
                        *logger << Logger::LogLevel::ERROR << msg << Logger::LogAction::FLUSH;
                        throw SSLError(__file__, __LINE__, msg);
                    }
                    if (SSL_set_fd(cSSL, newsockfs) != 1) {
                        string msg = "OpenSSL error: 'SSL_set_fd()' failed!";
                        *logger << Logger::LogLevel::ERROR << msg << Logger::LogAction::FLUSH;
                        throw SSLError(__file__, __LINE__, msg);
                    }

                    //Here is the SSL Accept portion.  Now all reads and writes must use SS
                    int suc;
                    if ((suc = SSL_accept(cSSL)) <= 0) {
                        string msg = "OpenSSL error: 'SSL_accept()' failed!";
                        *logger << Logger::LogLevel::ERROR << msg << Logger::LogAction::FLUSH;
                        throw SSLError(__file__, __LINE__, msg);
                    }
               }
                catch (SSLError &err) {
                    *logger << Logger::LogLevel::ERROR << err.to_string() << Logger::LogAction::FLUSH;
                    int sstat;
                    while ((sstat = SSL_shutdown(cSSL)) == 0);
                    if (sstat < 0) {
                        *logger << Logger::LogLevel::WARNING << "SSL socket error: shutdown (2) of socket failed! Reason: " << SSL_get_error(cSSL, sstat) << Logger::LogAction::FLUSH;
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
                        *logger << Logger::LogLevel::DEBUG << "Thread to stop does no longer exist...!" << Logger::LogAction::FLUSH;
                    }
                }
                else {
                    idlelock.unlock();
                }
            }
            semaphore_wait();
            int commpipe[2];

            if (socketpair(PF_LOCAL, SOCK_STREAM, 0, commpipe) != 0) {
                *logger << Logger::LogLevel::WARNING << "Creating pipe failed! ERROR: " << strerror(errno) << Logger::LogAction::FLUSH;
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
                *logger << Logger::LogLevel::ERROR << "Could not create thread " << strerror(errno) << Logger::LogAction::FLUSH;
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
        *logger << Logger::LogLevel::INFORMATIONAL << Logger::LogAction::FORCE << "Server shutting down!" << Logger::LogAction::FLUSH;

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
        auto logger = Logger::getLogger(loggername);
        if (_tmpdir.empty()) {
            *logger << Logger::LogLevel::WARNING << "_tmpdir is empty" << Logger::LogAction::FLUSH;
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
            LuaServer luaserver(conn, _initscript, true);
            luaserver.setLuaPath(_scriptdir + "/?.lua"); // add the script dir to the standard search path fpr lua packages
            //luaserver.createGlobals(conn);
            for (auto &global_func : lua_globals) {
                global_func.func(luaserver.lua(), conn, global_func.func_dataptr);
            }

            void *hd = NULL;
            try {
                RequestHandler handler = getHandler(conn, &hd);
                handler(conn, luaserver, _user_data, hd);
            }
            catch (int i) {
                *logger << Logger::LogLevel::DEBUG << "Possibly socket closed by peer!" << Logger::LogAction::FLUSH;
                return CLOSE; // or CLOSE ??
            }
            if (!conn.cleanupUploads()) {
                *logger << Logger::LogLevel::ERROR << "Cleanup of uploaded files failed!" << Logger::LogAction::FLUSH;
            }
            if (conn.keepAlive()) {
                return CONTINUE;
            }
            else {
                return CLOSE;
            }
        }
        catch (int i) { // "error" is thrown, if the socket was closed from the main thread...
            *logger << Logger::LogLevel::DEBUG << "Socket connection: timeout or socket closed from main" << Logger::LogAction::FLUSH;
            return CLOSE;
        }
        catch(Error &err) {
            try {
                *logger << Logger::LogLevel::DEBUG << "Internal server error: " << err << Logger::LogAction::FLUSH;
                *os << "HTTP/1.1 500 INTERNAL_SERVER_ERROR\r\n";
                *os << "Content-Type: text/plain\r\n";
                stringstream ss;
                ss << err;
                *os << "Content-Length: " << ss.str().length() << "\r\n\r\n";
                *os << ss.str();
            }
            catch (int i) {
                *logger << Logger::LogLevel::DEBUG << "Possibly socket closed by peer!"  << Logger::LogAction::FLUSH;
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
