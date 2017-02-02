--
-- Copyright © 2016 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
-- Ivan Subotic, Tobias Schweizer, André Kilchenmann, and André Fatton.
-- This file is part of Sipi.
-- Sipi is free software: you can redistribute it and/or modify
-- it under the terms of the GNU Affero General Public License as published
-- by the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
-- Sipi is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
-- Additional permission under GNU AGPL version 3 section 7:
-- If you modify this Program, or any covered work, by linking or combining
-- it with Kakadu (or a modified version of that library), containing parts
-- covered by the terms of the Kakadu Software Licence, the licensors of this
-- Program grant you additional permission to convey the resulting work.
-- See the GNU Affero General Public License for more details.
-- You should have received a copy of the GNU Affero General Public
-- License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------------
-- String constants to be returned
-------------------------------------------------------------------------------
TEXT = "text"
IMAGE = "image"

-------------------------------------------------------------------------------
-- Mimetype constants
-------------------------------------------------------------------------------

XML = "application/xml"
PLAIN_TEXT = "plain/text"

-------------------------------------------------------------------------------
-- This function is called from the route to determine the media type (image, text file) of a given file.
-- Parameters:
-- 'mimetype' (string):  the mimetype of the file.
--
-- Returns:
-- the media type of the file or false in case no supported type could be determined.
-------------------------------------------------------------------------------
function get_mediatype(mimetype)

    if mimetype == "application/xml" or mimetype == "text/plain" then
        return TEXT

    elseif mimetype == "image/jp2" or mimetype == "image/tiff" or mimetype == "image/png" or mimetype == "image/jpeg" then

        return IMAGE

        -- TODO: implement video and audio

    else

        -- no supported mediatype could be determined
        return false
    end
end

-------------------------------------------------------------------------------
-- This function is called from the route to check the file extension of the given filename.
-- Parameters:
-- 'mimetype' (string):  the mimetype of the file.
-- `filename` (string): the name of the file excluding the file extension.
--
-- Returns:
-- a boolean indicating whether the file extension is correct or not.
-------------------------------------------------------------------------------
function check_file_extension(mimetype, filename)

    if (mimetype == XML) then
        local ext = string.sub(filename, -4)

        -- valid extensions are: xml, xsl (XSLT), and .xsd (XML Schema)
        return ext == ".xml" or ext == ".xsl" or ext == ".xsd"
    end
end


-------------------------------------------------------------------------------
-- This function splits a string containing a mimetype and a charset into a pair.
-- The information was sent by the client.
-- Parameters:
-- 'mimetype_and_charset' (string): the mimetype and charset of a file, e.g. "application/xml; charset=UTF-8"
--
-- Returns:
-- a pair containing (first) the mimetype, and (second) the charset or false if no charset is given
function split_mimetype_and_charset(mimetype_and_charset)

    -- check if mimetype_and_charset contains a semicolon followed by a whitespace character
    -- mimetype_and_charset might look like this: "application/xml; charset=UTF-8"
    -- string.find returns 'nil' if the pattern was not found
    local start_char, end_char = string.find(mimetype_and_charset, "; ")

    -- make sure string.find did not return 'nil' and both start and end characters have been returned
    if start_char ~= nil and (start_char + 1) == end_char then
        -- pattern "; " was found in string

        -- original mimetype also contains the encoding, e.g. "application/xml; charset=UTF-8"
        -- split the string using the position information start_char and end_char

        -- get a substring from the beginning until the position of "; "
        -- substract 1 from start_char because Lua includes the last character which we do not want
        local mimetype = string.sub(mimetype_and_charset, 1, start_char -1) -- e.g. "application/xml"

        -- get a substring from the end of "; " until the last character
        -- add 1 to end_char because otherwise Lua includes the whitespace which we do not want
        local charset = string.sub(mimetype_and_charset, end_char +1) -- e.g. "charset=UTF-8"

        -- in summary, Lua strings start with 1 (and not 0)
        -- and the numbers refer to characters and not positions (this is why the the end position includes the ending character)
        -- see https://www.lua.org/pil/20.html

        return mimetype, charset

    else
        -- pattern "; " was not found

        -- assuming that only the mimetype is given, but no charset
        return mimetype_and_charset, false
    end
end
