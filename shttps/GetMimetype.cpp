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

#include <regex>
#include <sstream>

#include "GetMimetype.h"
#include "Error.h"

#include "magic.h"

static const char __file__[] = __FILE__;

namespace shttps {

    namespace GetMimetype {
        // A regex for parsing the value of an HTTP Content-Type header.
        std::regex mime_regex("^([^;]+)(;\\s*charset=\"?([^\"]+)\"?)?$", std::regex_constants::ECMAScript | std::regex_constants::icase);

        std::pair<std::string, std::string> parseMimetype(const std::string& mimestr) {
            std::smatch mime_match;
            std::string mimetype;
            std::string charset;

            if (std::regex_match(mimestr, mime_match, mime_regex)) {
                if (mime_match.size() > 1) {
                    mimetype = mime_match[1].str();

                    if (mime_match.size() == 4) {
                        charset = mime_match[3].str();
                    }
                }
            } else {
                std::ostringstream error_msg;
                throw Error(__file__, __LINE__, error_msg.str());
            }

            // Convert MIME type and charset to lower case
            std::transform(mimetype.begin(), mimetype.end(), mimetype.begin(), ::tolower);
            std::transform(charset.begin(), charset.end(), charset.begin(), ::tolower);

            return std::make_pair(mimetype, charset);
        }

        std::pair<std::string, std::string> getFileMimetype(const std::string& fpath) {
            magic_t handle;
            if ((handle = magic_open(MAGIC_MIME | MAGIC_PRESERVE_ATIME)) == nullptr) {
                throw Error(__file__, __LINE__, magic_error(handle));
            }

            if (magic_load(handle, nullptr) != 0) {
                throw Error(__file__, __LINE__, magic_error(handle));
            }

            std::string mimestr(magic_file(handle, fpath.c_str()));
            return parseMimetype(mimestr);
        }
    }
}
