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

#ifndef __shttps_global_h
#define __shttps_global_h

#include <string>
#include <sstream>
#include <istream>
#include <utility>
#include <string>
#include <iostream>


 namespace shttps {

    extern const char loggername[];

    template <typename Enumeration>
    inline auto as_integer(Enumeration const value)
        -> typename std::underlying_type<Enumeration>::type
    {
        return static_cast<typename std::underlying_type<Enumeration>::type>(value);
    }
    //-------------------------------------------------------------------------

    template <class OutIt>
    void explode(std::string const &input, char sep, OutIt output) {
        std::istringstream buffer(input);
        std::string temp;

        while (std::getline(buffer, temp, sep)) {
            *output++ = temp;
        }
    }
    //-------------------------------------------------------------------------


    inline std::string getFileName(const std::string& s) {
       char sep = '/';
       size_t i = s.rfind(sep, s.length());
       if (i != std::string::npos) {
          return(s.substr(i+1, s.length() - i));
       }
       return("");
    }
}

#endif
