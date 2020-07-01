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

#include "Parsing.h"
#include "Error.h"

#include "magic.h"

static const char __file__[] = __FILE__;

namespace shttps {

    namespace Parsing {

        static std::unordered_map <std::string, std::unordered_set<std::string>> mimetypes = {
                {"jpx",     {"image/jp2", "image/jpx"}},
                {"jp2",     {"image/jp2", "image/jpx"}},
                {"jpg",     {"image/jpeg"}},
                {"jpeg",    {"image/jpeg"}},
                {"tiff",    {"image/tiff"}},
                {"tif",     {"image/tiff"}},
                {"png",     {"image/png"}},
                {"webp",    {"image/webp"}},
                {"bmp",     {"image/x-ms-bmp"}},
                {"gif",     {"image/gif"}},
                {"ico",     {"image/x-icon"}},
                {"pnm",     {"image/x-portable-anymap"}},
                {"pbm",     {"image/x-portable-bitmap"}},
                {"pgm",     {"image/x-portable-graymap"}},
                {"ppm",     {"image/x-portable-pixmap"}},
                {"svg",     {"image/svg+xml"}},
                {"pdf",     {"application/pdf"}},
                {"dwg",     {"application/acad"}},
                {"gz",      {"application/gzip"}},
                {"json",    {"application/json"}},
                {"js",      {"application/javascript"}},
                {"xls",     {"application/msexcel",                                               "application/vnd.ms-excel"}},
                {"xla",     {"application/msexcel",                                               "application/vnd.ms-excel"}},
                {"xlsx",
                            {"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", "application/vnd.ms-excel"}},
                {"ppt",     {"application/mspowerpoint",                                          "application/vnd.ms-powerpoint"}},
                {"pptx",    {"application/mspowerpoint",                                          "application/vnd.ms-powerpoint"}},
                {"pps",     {"application/mspowerpoint",                                          "application/vnd.ms-powerpoint"}},
                {"pot",     {"application/mspowerpoint",                                          "application/vnd.ms-powerpoint"}},
                {"doc",     {"application/msword"}},
                {"docx",    {"application/vnd.openxmlformats-officedocument.wordprocessingml.document"}},
                {"bin",     {"application/octet-stream"}},
                {"dat",     {"application/octet-stream"}},
                {"ps",      {"application/postscript"}},
                {"eps",     {"application/postscript"}},
                {"ai",      {"application/postscript"}},
                {"rtf",     {"application/rtf",                                                   "text/rtf"}},
                {"htm",     {"text/html",                                                         "application/xhtml+xml"}},
                {"html",    {"text/html",                                                         "application/xhtml+xml"}},
                {"shtml",   {"text/html",                                                         "application/xhtml+xml"}},
                {"xhtml",   {"application/xhtml+xml"}},
                {"css",     {"text/css"}},
                {"xml",     {"application/xml",                                                   "text/xml"}},
                {"z",       {"application/x-compress"}},
                {"tgz",     {"application/x-compress"}},
                {"dvi",     {"application/x-dvi"}},
                {"gtar",    {"application/x-gtar"}},
                {"hdf",     {"application/x-hdf"}},
                {"php",     {"application/x-httpd-php"}},
                {"phtml",   {"application/x-httpd-php"}},
                {"tex",     {"application/x-tex",                                                 "application/x-latex"}},
                {"latex",   {"application/x-latex"}},
                {"texi",    {"application/x-texinfo"}},
                {"texinfo", {"application/x-texinfo"}},
                {"tar",     {"application/x-tar"}},
                {"zip",     {"application/zip"}},
                {"au",      {"audio/basic"}},
                {"snd",     {"audio/basic"}},
                {"mp3",     {"audio/mpeg"}},
                {"mp4",     {"audio/mp4"}},
                {"ogg",     {"audio/ogg",                                                         "video/ogg", "application/ogg"}},
                {"ogv",     {"video/ogg",                                                         "application/ogg"}},
                {"wav",     {"audio/wav",                                                         "audio/wav", "audio/x-wav", "audio/x-pn-wav"}},
                {"aif",     {"audio/x-aiff"}},
                {"aiff",    {"audio/x-aiff"}},
                {"aifc",    {"audio/x-aiff"}},
                {"mid",     {"audio/x-midi"}},
                {"midi",    {"audio/x-midi"}},
                {"mp2",     {"audio/x-mpeg"}},
                {"wrl",     {"model/vrml",                                                        "x-world/x-vrml"}},
                {"ics",     {"text/calendar"}},
                {"csv",     {"text/comma-separated-values",                                       "text/plain"}},
                {"tsv",     {"text/tab-separated-values",                                         "text/plain"}},
                {"txt",     {"text/plain"}},
                {"rtx",     {"text/richtext"}},
                {"sgm",     {"text/x-sgml"}},
                {"sgml",    {"text/x-sgml"}},
                {"mpeg",    {"video/mpeg"}},
                {"mpg",     {"video/mpeg"}},
                {"mpe",     {"video/mpeg"}},
                {"mp4",     {"video/mp4"}},
                {"mov",     {"video/quicktime"}},
                {"qt",      {"video/quicktime"}},
                {"viv",     {"video/vnd.vivo"}},
                {"vivo",    {"video/vnd.vivo"}},
                {"webm",    {"video/webm"}},
                {"avi",     {"video/x-msvideo"}},
                {"3gp",     {"video/3gpp"}},
                {"movie",   {"video/x-sgi-movie"}},
                {"swf",     {"application/x-shockwave-flash"}},
                {"vcf",     {"text/x-vcard"}}};

        std::pair <std::string, std::string> parseMimetype(const std::string &mimestr) {
            try {
                // A regex for parsing the value of an HTTP Content-Type header. In C++11, initialization of this
                // static local variable happens once and is thread-safe.
                static std::regex mime_regex("^([^;]+)(;\\s*charset=\"?([^\"]+)\"?)?$",
                                             std::regex_constants::ECMAScript | std::regex_constants::icase);

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
                    error_msg << "Could not parse MIME type: " << mimestr;
                    throw Error(__file__, __LINE__, error_msg.str());
                }

                // Convert MIME type and charset to lower case
                std::transform(mimetype.begin(), mimetype.end(), mimetype.begin(), ::tolower);
                std::transform(charset.begin(), charset.end(), charset.begin(), ::tolower);

                return std::make_pair(mimetype, charset);
            } catch (std::regex_error &e) {
                std::ostringstream error_msg;
                error_msg << "Regex error: " << e.what();
                throw Error(__file__, __LINE__, error_msg.str());
            }
        }

        std::pair <std::string, std::string> getFileMimetype(const std::string &fpath) {
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


        bool checkMimeTypeConsistency(const std::string &path) {
            return checkMimeTypeConsistency(path, path);
        }

        bool checkMimeTypeConsistency(
                const std::string &path,
                const std::string &filename,
                const std::string &given_mimetype) {
            try {
                //
                // first we get the possible mimetypes associated with the given filename extension
                //
                size_t dot_pos = filename.find_last_of(".");
                if (dot_pos == std::string::npos) {
                    std::stringstream ss;
                    ss << "Invalid filename without extension: \"" << filename << "\"";
                    throw Error(__file__, __LINE__, ss.str());
                }
                std::string extension = filename.substr(dot_pos + 1);
                // convert file extension to lower case (uppercase letters in file extension have to be converted for
                // mime type comparison)
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                std::unordered_set<std::string> mime_from_extension = mimetypes.at(extension);

                //
                // now we get the mimetype determined by the magic number
                //
                std::pair <std::string, std::string> actual_mimetype = getFileMimetype(path);

                //
                // now we test if the mimetype given given by the magic number corresponds to a valid mimetype for this extension
                //
                std::unordered_set<std::string>::const_iterator got1 = mime_from_extension.find(actual_mimetype.first);
                if (got1 == mime_from_extension.end()) {
                    return false;
                }

                if (!given_mimetype.empty()) {
                    //
                    // now we test if the expected mimetype corresponds to a valid mimetype for this extension
                    //
                    std::unordered_set<std::string>::const_iterator got2 = mime_from_extension.find(given_mimetype);
                    if (got2 == mime_from_extension.end()) {
                        return false;
                    }
                }
            } catch (std::out_of_range &e) {
                std::stringstream ss;
                ss << "Unsupported file type: \"" << filename << "\"";
                throw shttps::Error(__file__, __LINE__, ss.str());
            }

            return true;
        }

        size_t parse_int(std::string &str) {
            try {
                // A regex for parsing an integer containing only digits. In C++11, initialization of this
                // static local variable happens once and is thread-safe.
                static std::regex int_regex("^[0-9]+$", std::regex_constants::ECMAScript);
                std::smatch int_match;

                if (std::regex_match(str, int_match, int_regex)) {
                    std::stringstream sstream(int_match[0]);
                    size_t result;
                    sstream >> result;
                    return result;
                } else {
                    std::ostringstream error_msg;
                    error_msg << "Could not parse integer: " << str;
                    throw Error(__file__, __LINE__, error_msg.str());
                }
            } catch (std::regex_error &e) {
                std::ostringstream error_msg;
                error_msg << "Regex error: " << e.what();
                throw Error(__file__, __LINE__, error_msg.str());
            }
        }

        float parse_float(std::string &str) {
            try {
                // A regex for parsing a floating-point number containing only digits and an optional decimal point. In C++11,
                // initialization of this static local variable happens once and is thread-safe.
                static std::regex float_regex("^[0-9]+(\\.[0-9]+)?$", std::regex_constants::ECMAScript);
                std::smatch float_match;

                if (std::regex_match(str, float_match, float_regex)) {
                    std::stringstream sstream(float_match[0]);
                    float result;
                    sstream >> result;
                    return result;
                } else {
                    std::ostringstream error_msg;
                    error_msg << "Could not parse floating-point number: " << str;
                    throw Error(__file__, __LINE__, error_msg.str());
                }
            } catch (std::regex_error &e) {
                std::ostringstream error_msg;
                error_msg << "Regex error: " << e.what();
                throw Error(__file__, __LINE__, error_msg.str());
            }
        }


    }
}
