
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
-- it with Kakadu (or a modified version of that library) or Adobe ICC Color
-- Profiles (or a modified version of that library) or both, containing parts
-- covered by the terms of the Kakadu Software Licence or Adobe Software Licence,
-- or both, the licensors of this Program grant you additional permission
-- to convey the resulting work.
-- You should have received a copy of the GNU Affero General Public
-- License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.
--
-------------------------------------------------------------------------------
-- This function is being called from sipi before the file is served
-- Parameters:
--    prefix: This is the prefix that is given on the IIIF url
--    identifier: the identifier for the image
--    cookie: The cookie that may be present
--
-- Returns:
--    permission:
--       'allow' : the view is allowed with the given IIIF parameters
--       'restrict:watermark=<path-to-watermark>' : Add a watermark
--       'restrict:size=<iiif-size-string>' : reduce size/resolution
--       'deny' : no access!
--    filepath: server-path where the master file is located
-------------------------------------------------------------------------------
function pre_flight(prefix,identifier,cookie)

    --for key, name in pairs(server.header) do
    --    print(key, " -> ", name)
    --end


    if config.prefix_as_path then
        filepath = config.imgroot .. '/' .. prefix .. '/' .. identifier
    else
        filepath = config.imgroot .. '/' .. identifier
    end



    return 'allow', filepath

end
-------------------------------------------------------------------------------
