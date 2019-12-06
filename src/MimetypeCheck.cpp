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

#include <iostream>
#include <string>
#include "shttps/Global.h"
#include "shttps/Parsing.h"
#include "MimetypeCheck.h"

static const char __file__[] = __FILE__;

namespace Sipi {

    std::unordered_map<std::string, std::string> MimetypeCheck::mimetypes = {{"jpx",  "image/jp2"},
                                                                         {"jp2",  "image/jp2"},
                                                                         {"jpg",  "image/jpeg"},
                                                                         {"jpeg", "image/jpeg"},
                                                                         {"tiff", "image/tiff"},
                                                                         {"tif",  "image/tiff"},
                                                                         {"png",  "image/png"},
                                                                         {"pdf",  "application/pdf"}};

    /*!
     * This function compares the actual mime type of a file (based on its magic number) to
     * the given mime type (sent by the client) and the extension of the given filename (sent by the client)
     */
    bool MimetypeCheck::checkMimeTypeConsistency(const std::string &path, const std::string &given_mimetype,
                                             const std::string &filename) {
        try {
            std::string actual_mimetype = shttps::Parsing::getFileMimetype(path).first;

            if (actual_mimetype != given_mimetype) {
                //std::cerr << actual_mimetype << " does not equal " << given_mimetype << std::endl;
                return false;
            }

            size_t dot_pos = filename.find_last_of(".");

            if (dot_pos == std::string::npos) {
                //std::cerr << "invalid filename " << filename << std::endl;
                return false;
            }

            std::string extension = filename.substr(dot_pos + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); // convert file extension to lower case (uppercase letters in file extension have to be converted for mime type comparison)
            std::string mime_from_extension = Sipi::MimetypeCheck::mimetypes.at(extension);

            if (mime_from_extension != actual_mimetype) {
                //std::cerr << "filename " << filename << "has not mime type " << actual_mimetype << std::endl;
                return false;
            }
        } catch (std::out_of_range &e) {
            std::stringstream ss;
            ss << "Unsupported file type: \"" << filename;
            //throw SipiImageError(__file__, __LINE__, ss.str());
        } /*catch (shttps::Error &err) {
            throw SipiImageError(__file__, __LINE__, err.to_string());
        }*/

        return true;
    }

}
