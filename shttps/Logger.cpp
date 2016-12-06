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
#include "Global.h"
#include "Logger.h"

#include <fstream>      // std::filebuf
#include <iostream>
#include <iomanip>
#include <ctime>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


using namespace std;


static map<Logger::LogLevel,string> LogLevelNames = {
    {Logger::EMERGENCY,     "EMERGENCY"},
    {Logger::ALERT,         "ALERT"},
    {Logger::CRITICAL,      "CRITICAL"},
    {Logger::ERROR,         "ERROR"},
    {Logger::WARNING,       "WARNING"},
    {Logger::NOTICE,        "NOTICE"},
    {Logger::INFORMATIONAL, "INFORMATIONAL"},
    {Logger::DEBUG,         "DEBUG"}
};

static std::map<std::string,shared_ptr<Logger>> loggers;


shared_ptr<Logger> Logger::createLogger(const std::string &name_p, const std::string &filename, LogLevel loglevel) {
    shared_ptr<Logger> tmp;
    LogStream *ls = new LogStream(filename);
    loggers[name_p] = tmp = shared_ptr<Logger>(new Logger(name_p, ls, loglevel));
    return tmp;
}

shared_ptr<Logger> Logger::getLogger(const string &name) {
    shared_ptr<Logger> logger;
    try {
        logger = loggers.at(name);
    }
    catch (const std::out_of_range& oor) {
        return NULL;
    }
    return logger;
}

void Logger::removeLogger(const string &name) {
    shared_ptr<Logger> logger;
    try {
        logger = loggers.at(name);
    }
    catch (const std::out_of_range& oor) {
        return;
    }
    loggers.erase(name);
    logger.reset();
}

Logger::~Logger() {
    delete ls;
}


map<Logger::LogLevel,string> Logger::getLevelMap(void) {
    return LogLevelNames;
}

Logger& Logger::operator<< (LogAction action) {
    switch (action) {
        case LogAction::FLUSH: {
            if (msg_loglevel <= loglevel || force) {
                std::ostream::operator<<(endl);
                flush();
            }
            force = false;
            header = false;
            active.unlock();
            break;
        }
        case LogAction::FORCE: {
            force = true;
            if (!header) {
                header = true;
                auto t = std::time(nullptr);
                auto tm = *std::localtime(&t);
                *this << put_time(&tm, "[%Y-%m-%d %H-%M-%S] [") << name << ":" << LogLevelNames[msg_loglevel] << "] ";
            }
            break;
        }
    }
    return *this;
}

Logger& Logger::operator<< (LogLevel ll) {
    active.lock();
    msg_loglevel = ll;
    if (msg_loglevel <= loglevel) {
        header = true;
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        *this << put_time(&tm, "[%Y-%m-%d %H-%M-%S] [") << name << ":" << LogLevelNames[msg_loglevel] << "] ";
    }
    return *this;
}

Logger& Logger::operator<< (bool val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (char val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (unsigned char val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (short val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (unsigned short val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (int val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (unsigned int val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (long val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (unsigned long val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (long long val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (unsigned long long val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (float val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (double val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (long double val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}

Logger& Logger::operator<< (void *val) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(val);
    return *this;
}


Logger& Logger::operator<< (std::streambuf* sb ) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(sb);
    return *this;
}

Logger& Logger::operator<< (std::ostream& (*pf)(std::ostream&)) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(pf);
    return *this;
}


Logger& Logger::operator<< (std::ios& (*pf)(std::ios&)) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(pf);
    return *this;
}


Logger& Logger::operator<< (ios_base& (*pf)(ios_base&)) {
    if (msg_loglevel <= loglevel || force) std::ostream::operator<<(pf);
    return *this;
}


Logger& Logger::operator<< (const std::string &val) {
    if (msg_loglevel <= loglevel || force) {
        ostream *os = this;
        *os << val;
    }
    return *this;
}

Logger& Logger::operator<< (const char *str) {
    if (msg_loglevel <= loglevel || force) {
        ostream *os = this;
        *os << str;
    }
    return *this;
}

Logger& Logger::operator<< (const shttps::Error &err) {
    ostream *os = this;
    *os << err;
    return *this;
}
