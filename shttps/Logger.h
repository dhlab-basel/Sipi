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
 * \brief Implements socket handling as iostreams.
 */
#ifndef __shttp_logger_h
#define __shttp_logger_h


#include <ostream>
#include <streambuf>
#include <ios>
#include <map>
#include <mutex>
#include <vector>

#include "LogStream.h"

class Logger : public std::ostream {
public:
    typedef enum { // following list of severities of syslogd, also defined by https://tools.ietf.org/html/rfc3164
        EMERGENCY = 0,
        ALERT = 1,
        CRITICAL = 2,
        ERROR = 3,
        WARNING = 4,
        NOTICE = 5,
        INFORMATIONAL = 6,
        DEBUG = 7
    } LogLevel;
    typedef enum {
        FLUSH
    } LogAction;
private:

    LogStream *ls;
    LogLevel loglevel;
    LogLevel msg_loglevel;
    std::mutex active;
public:
    static Logger *createLogger(const std::string &name, const std::string &filename, LogLevel loglevel);
    static Logger *getLogger(const std::string &name);
    static void removeLogger(const std::string &name);

    inline Logger(LogStream *ls_p, LogLevel loglevel_p) : std::ostream(ls_p), ls(ls_p), loglevel(loglevel_p) {};
    ~Logger();

    Logger& operator<< (LogLevel ll);
    Logger& operator<< (LogAction action);
    Logger& operator<< (bool val);
    Logger& operator<< (char val);
    Logger& operator<< (unsigned char val);
    Logger& operator<< (short val);
    Logger& operator<< (unsigned short val);
    Logger& operator<< (int val);
    Logger& operator<< (unsigned int val);
    Logger& operator<< (long val);
    Logger& operator<< (unsigned long val);
    Logger& operator<< (long long val);
    Logger& operator<< (unsigned long long val);
    Logger& operator<< (float val);
    Logger& operator<< (double val);
    Logger& operator<< (long double val);
    Logger& operator<< (void *val);

    Logger& operator<< (std::streambuf* sb );
    Logger& operator<< (std::ostream& (*pf)(std::ostream&));
    Logger& operator<< (std::ios& (*pf)(std::ios&));
    Logger& operator<< (ios_base& (*pf)(ios_base&));

    Logger& operator<< (const std::string &val);
    Logger& operator<< (const char *str);
};
#endif
