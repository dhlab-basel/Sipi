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
#include <string>
#include <ios>
#include <map>
#include <mutex>
#include <vector>
#include <memory>

#include "Error.h"
#include "LogStream.h"

/*!
* This as a (for now) primitive class which can be used to log messages in a
* multithreaded environment. Currently only logging to files is possible, but in
* the future other mechanisms (e.g. syslogd) may be implemented.
*
* Logging is done through the normal stream operator "<<". In order to start a
* logging message, the severity level has to be the first item. To end the logging
* message Logger::FLUSH has to be added!
*
* Example:
*
* \code{.cpp}
* auto logger = shttps::Logger:getLogger("loggername");
* logger << shttps::Logger::ERROR << "this is an error you never will see!" << shttps::Logger::FLUSH;
* \endcode
*/
class Logger : public std::ostream {
public:
    typedef enum { //!< following list of severities of syslogd, also defined by https://tools.ietf.org/html/rfc3164
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
        FLUSH,
        FORCE
    } LogAction;
private:
    std::string name; //!< name of the logger under which it can be found be getLogger()
    LogStream *ls; //!< Logstream. Currently only filestreams (LogStream) are implemented
    LogLevel loglevel; //!< From this loglevel on messages will be logged
    LogLevel msg_loglevel; //!< loglevel of the actual message
    bool force; //!< true, if a messages should be logged even if it is no severe enough
    bool header;
    std::mutex active; //!< Mutex for securing messages in multithreaded environments
public:
   /*!
    * Logger factory which creates a new logger with the given name and returns an instance of it
    *
    * \param[in] name_p Name of the logger
    * \param[in] filename Path of the logging file
    * \param[in] loglevel The log level of the logger. Message with a severity more or equal will be logged
    *
    * \returns logger instance
    */
    static std::shared_ptr<Logger> createLogger(const std::string &name_p, const std::string &filename, LogLevel loglevel);

   /*!
    * Get a instance of an already created logger
    *
    * \param[in] name Name of the existing logger
    *
    * \returns logger instance
    */
    static std::shared_ptr<Logger> getLogger(const std::string &name);

   /*!
    * Removes a logger and destroys all associated resources and closes the file
    *
    * \param[in] name Name of the logger to be destroyed
    */
    static void removeLogger(const std::string &name);

   /*!
    * Constructor of the Logger.
    * \note This constructor should not be used directly. Use createLogger() instead!
    *
    * \param[in] name_p Name of the logger
    * \param[in] ls_p LogStream Instance
    * \param[in] loglevel_p Logging level of the newly created logger
    *
    */
    inline Logger(const std::string &name_p, LogStream *ls_p, LogLevel loglevel_p)
    : std::ostream(ls_p), name(name_p), ls(ls_p), loglevel(loglevel_p) {
        force = false;
        header = false;
    };

   /*!
    * Destroy the logger
    * \note Should not use directly. Use removeLogger() instead!
    */
    ~Logger();

   /*!
    * Change the log level of the logger
    *
    * \param[in] level The new log level of the type LogLevel
    */
    inline void setLoglevel(LogLevel level) { loglevel = level; };

   /*!
    * Return a map of log level names
    */
    static std::map<Logger::LogLevel,std::string> getLevelMap(void);

    /*!
    * Output operator for starting a log message
    * \code{.cpp}
    * logger << shttps::Logger::ERROR …
    * \endcode
    *
    * \param[in] ll log level of message
    */
    Logger& operator<< (LogLevel ll);

    /*!
    * Output operator for actions
    * \code{.cpp}
    * logger << shttps::Logger::INFORMATIONAL << SHTTPS::LOGGER::FORCE << "I alwasy want to see this" << shttps::Logger::FLUSH;
    * \endcode
    *
    * \param[in] action Logger action
    */
    Logger& operator<< (LogAction action);

    /*!
    * Output operator for boolean
    */
    Logger& operator<< (bool val);

    /*!
    * Output operator for char
    */
    Logger& operator<< (char val);

    /*!
    * Output operator for unsigned char
    */
    Logger& operator<< (unsigned char val);

    /*!
    * Output operator short
    */
    Logger& operator<< (short val);

    /*!
    * Output operator unsigend short
    */
    Logger& operator<< (unsigned short val);

    /*!
    * Output operator for int
    */
    Logger& operator<< (int val);

    /*!
    * Output operator for unsigned int
    */
    Logger& operator<< (unsigned int val);

    /*!
    * Output operator for long
    */
    Logger& operator<< (long val);

    /*!
    * Output operator for unsigned long
    */
    Logger& operator<< (unsigned long val);

    /*!
    * Output operator for long long
    */
    Logger& operator<< (long long val);

    /*!
    * Output operator for unsigned long long
    */
    Logger& operator<< (unsigned long long val);

    /*!
    * Output operator for float
    */
    Logger& operator<< (float val);

    /*!
    * Output operator for double
    */
    Logger& operator<< (double val);

    /*!
    * Output operator for long double
    */
    Logger& operator<< (long double val);

    /*!
    * Output operator pointers
    */
    Logger& operator<< (void *val);

    /*!
    * Output operator for streambuffers
    */

    Logger& operator<< (std::streambuf* sb );

    /*!
    * Output operator for streams
    */
    Logger& operator<< (std::ostream& (*pf)(std::ostream&));

    /*!
    * Output operator format operators
    */
    Logger& operator<< (std::ios& (*pf)(std::ios&));

    /*!
    * Output operator
    */
    Logger& operator<< (ios_base& (*pf)(ios_base&));

    /*!
    * Output operator for c++ strings
    */
    Logger& operator<< (const std::string &val);

    /*!
    * Output operator for c-strings (NULL terminated!)
    */
    Logger& operator<< (const char *str);

    /*!
    * Output operator for Error
    */
    Logger& operator<< (const shttps::Error &err);

};
#endif
