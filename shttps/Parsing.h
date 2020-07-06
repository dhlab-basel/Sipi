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
#ifndef __shttps_parsing_h
#define __shttps_parsing_h

#include <string>
#include <regex>
#include <unordered_map>
#include <unordered_set>

namespace shttps {

    /*!
     * \brief Parsing utilities.
     */
    namespace Parsing {
        //static std::unordered_map<std::string, std::string> mimetypes; //! format (key) to mimetype (value) conversion map

        /*!
         * Parses a string containing a MIME type and optional character set, such as the Content-Type header defined by
         * <https://tools.ietf.org/html/rfc7231#section-3.1.1>.
         * @param mimestr a string containing the MIME type.
         * @return the MIME type and optional character set.
         */
        std::pair<std::string, std::string> parseMimetype(const std::string &mimestr);


        /*!
         * Determine the mimetype of a file using the magic number
         *
         * \param[in] fpath Path to file to check for the mimetype
         * \returns pair<string,string> containing the mimetype as first part
         *          and the charset as second part. Access as val.first and val.second!
         */
        std::pair<std::string, std::string> getFileMimetype(const std::string &fpath);

        bool checkMimeTypeConsistency(const std::string &path);

        /*!
         *
         * @param path Path to file (
         * @param filename Original filename with extension
         * @param given_mimetype The mimetype given by the browser
         * @return true, if consistent, else false
         */
        bool checkMimeTypeConsistency(
                const std::string &path,
                const std::string &filename,
                const std::string &given_mimetype = "");


        /*!
         * Parses an integer.
         * @param str the string to be parsed.
         * @return the corresponding integer.
         */
        size_t parse_int(std::string &str);

        /*!
         * Parses a floating-point number containing only digits and an optional decimal point.
         *
         * @param str the string to be parsed.
         * @return the corresponding floating-point number.
         */
        float parse_float(std::string &str);
    }
}

#endif //__shttps_parsing_h
