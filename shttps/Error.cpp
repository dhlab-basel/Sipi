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
 */#include <cstring>
#include <sstream>      // std::stringstream

#include "Error.h"

using namespace std;
namespace shttps {

    Error::Error (const char *file_p, const int line_p, const char *msg, int errno_p)
        : runtime_error(string(string(msg) + "\nFile: ") + string(file_p) + string(" Line: ") + std::to_string(line_p)),
    line (line_p),
    file (file_p),
    message (msg),
    sysErrno (errno_p)
    {

    }
    //============================================================================


    Error::Error (const char *file_p, const int line_p, const string &msg, int errno_p)
        : runtime_error(msg + string("\nFile: ") + string(file_p) + string(" Line: ") + std::to_string(line_p)),
    line (line_p),
    file (file_p),
    message (msg),
    sysErrno (errno_p)
    {

    }
    //============================================================================

    string Error::to_string(void)
    {
        stringstream ss;
        ss << "SHTTPS-ERROR at [" << file << ": " << line << "] ";
        if (sysErrno != 0) ss << "System error: " << strerror(sysErrno) << " ";
        ss << "Description: " << message;

        return ss.str();
    }
    //============================================================================

    ostream &operator<< (ostream &outstr, const Error &rhs)
    {
        outstr << endl << "SHTTPS-ERROR at [" << rhs.file << ": #" << rhs.line << "] " << endl;
        if (rhs.sysErrno != 0) {
            outstr << "System error: " << strerror(rhs.sysErrno) << endl;
        }
        outstr << "Description: " << rhs.message << endl;
        return outstr;
    }
    //============================================================================

}


