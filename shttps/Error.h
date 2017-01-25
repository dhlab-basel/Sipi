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
 * \brief Implements an error class for the http server.
 */
#ifndef __defined_shttps_error_h
#define __defined_shttps_error_h

#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>

namespace shttps {

   /*!
    * \brief Class used to thow errors from the web server implementation
    *
    * This class which inherits from \class std::runtime_error is used to throw catchable
    * errors from the web server. The error contains the cpp-file, line number, a user given
    * description and, if available, the system error message.
    */
    class Error: public std::runtime_error  {
    protected:
        int line;            //!< Linenumber where the exception has been throwns
        std::string file;    //!< Name of source code file where the exception has been thrown
        std::string message; //!< Description of the problem
        int sysErrno;        //!< If there is a system error number

    public:

        inline int getLine(void) const { return line; }
        inline std::string getFile(void) const { return file; }
        inline std::string getMessage(void) const { return message; }
        inline int getSysErrno(void) const { return sysErrno; }

        /*!
        * Constructor with all (char *) strings
        *
        * \param[in] file The filename, usually the __FILE__ macro.
        * \param[in] line The source code line, usually the __LINE__ macro.
        * \param[in] msg The message describing the error.
        * \param[in] errno_p Retrieve and display the system error message from errno.
        */
        Error (const char *file, const int line, const char *msg, int errno_p = 0);

        /*!
        * Constructor with std::string strings for the message. The file parameter is
        * is always of type (char *), becuase usually its either __LINE__ or a static
        * pointer to char
        *
        * \param[in] file The filename, usually the __FILE__ macro.
        * \param[in] line The source code line, usually the __LINE__ macro.
        * \param[in] msg The message describing the error.
        * \param[in] syserr Retrieve and display the system error message from errno.
        */
        Error (const char *file, const int line, const std::string &msg, int errno_p = 0);

       /*!
        * Retuns the error as a one-line string
        *
        * \returns Error string
        */
        std::string to_string(void) const;

        /*!
        * The overloaded << operator which is used to write the error message to the output
        *
        * \param[in] outStream The output stream
        * \param[in] rhs Reference to an instance of a Error
        * \returns Returns an std::ostream object
        */
        friend std::ostream &operator<< (std::ostream &outStream, const Error &rhs);
    };
}

#endif
