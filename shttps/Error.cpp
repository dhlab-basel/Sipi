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

#include <cstring>      // std::strerror
#include <sstream>      // std::ostringstream

#include "Error.h"

namespace shttps {

    Error::Error (const char *file_p, const int line_p, const char *msg, int errno_p)
        : runtime_error(std::string(msg) + "\nFile: " + std::string(file_p) + std::string(" Line: ") + std::to_string(line_p)),
    line (line_p),
    file (file_p),
    message (msg),
    sysErrno (errno_p)
    {

    }
    //============================================================================


    Error::Error (const char *file_p, const int line_p, const std::string &msg, int errno_p)
        : runtime_error(std::string(msg) + "\nFile: " + std::string(file_p) + std::string(" Line: ") + std::to_string(line_p)),
    line (line_p),
    file (file_p),
    message (msg),
    sysErrno (errno_p)
    {

    }
    //============================================================================

    std::string Error::to_string(void) const
    {
        std::ostringstream err_stream;
        err_stream << "Error at [" << file << ": " << line << "]";
        if (sysErrno != 0) err_stream << " (system error: " << std::strerror(sysErrno) << ")";
        err_stream << ": " << message;
        return err_stream.str();
    }
    //============================================================================

    std::ostream &operator<< (std::ostream &out_stream, const Error &rhs)
    {
        std::string errStr = rhs.to_string();
        out_stream << errStr << std::endl; // TODO: remove the endl, the logging code should do it
        return out_stream;
    }
    //============================================================================

}
