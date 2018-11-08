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

-- upload script for binary files (currently only images) from Knora
--

require "send_response"

function table.contains(table, element)
  for _, value in pairs(table) do
    if value == element then
      return true
    end
  end
  return false
end

myimg = {}
newfilename = {}
iiifurls = {}


for imgindex,imgparam in pairs(server.uploads) do
    --
    -- create a new Lua image object. This reads the image into an
    -- internal in-memory representation independent of the original
    -- image format.
    --
    success, myimg[imgindex] = SipiImage.from_upload(imgindex)
    if not success then
        server.log(myimg[imgindex], server.loglevel.error)
        send_error(500, myimg[imgindex])
        return false
    end

    newfilename[imgindex] = imgparam["origname"]:match "(.+)%..+" .. '.jp2'

    if server.secure then
        protocol = 'https://'
    else
        protocol = 'http://'
    end
    iiifurls[newfilename[imgindex]] = protocol .. server.host .. '/images/' .. newfilename[imgindex]
    iiifurls["filename"] = newfilename[imgindex]

    --
    -- here we add the subdirs that are necessary if Sipi is configured to use subdirs
    --
    success, newfilepath = helper.filename_hash(newfilename[imgindex]);
    if not success then
        send_error(500, newfilepath)
        return false
    end

    fullfilepath = config.imgroot .. '/' .. newfilepath

    local status, errmsg = myimg[imgindex]:write(fullfilepath)
    if not status then
        server.print('Error converting image to j2k: ', filename, ' ** ', errmsg)
    end
end

send_success(iiifurls)
