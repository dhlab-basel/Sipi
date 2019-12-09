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

#ifndef SIPI_MIMETYPECHECK_H
#define SIPI_MIMETYPECHECK_H

#include <iostream>
#include <unordered_map>
#include <sstream>
#include <string>
#include <exception>

namespace Sipi {

    class MimetypeCheck {

    public:
        static std::unordered_map<std::string, std::string> mimetypes; //! format (key) to mimetype (value) conversion map

        /*!
         * Checks if the actual mimetype of an image file corresponds to the indicated mimetype and the extension of the filename.
         * This function is used to check if information submitted with a file are actually valid.
         */
        static bool checkMimeTypeConsistency(const std::string &path, const std::string &given_mimetype,
                                             const std::string &filename);

    };

    class MimetypeCheckError : public std::exception {

    private:
        std::string file; //!< Source file where the error occurs in
        int line; //!< Line within the source file
        int errnum; //!< error number if a system call is the reason for the error
        std::string errmsg;

    public:
        /*!
        * Constructor
        * \param[in] file_p The source file name (usually __FILE__)
        * \param[in] line_p The line number in the source file (usually __LINE__)
        * \param[in] msg_p Error message describing the problem
        * \param[in] errnum_p Errnum, if a unix system call is the reason for throwing this exception
        */
        inline MimetypeCheckError(const char *file_p, int line_p, const std::string &msg_p, int errnum_p = 0) : file(
                file_p), line(line_p), errnum(errnum_p), errmsg(msg_p) {}

        inline std::string to_string(void) const {
            std::ostringstream errStream;
            errStream << "MimetypeCheckError at [" << file << ": " << line << "]";
            if (errnum != 0) errStream << " (system error: " << std::strerror(errnum) << ")";
            errStream << ": " << errmsg;
            return errStream.str();
        }
        //============================================================================

        inline friend std::ostream &operator<<(std::ostream &outStream, const MimetypeCheckError &rhs) {
            std::string errStr = rhs.to_string();
            outStream << errStr << std::endl; // TODO: remove the endl, the logging code should do it
            return outStream;
        }
        //============================================================================

    };
}

#endif
