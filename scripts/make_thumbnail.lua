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

-- Knora GUI-case: create a thumbnail

require "send_response"

success, errormsg = server.setBuffer()
if not success then
    return -1
end

--
-- check if tmporary directory is available, if not, create it
--
local tmpdir = config.imgroot .. '/tmp/'
local success, exists = server.fs.exists(tmpdir)
if not success then
    send_error(500, "Internal server error")
    return -1
end
if not exists then
    local success, result = server.fs.mkdir(tmpdir, 511)
    if not success then
        send_error(500, "Couldn't create tmpdir: " .. result)
        return
    end
end

if server.uploads == nil then
    send_error(500, "no image uploaded")
    return -1
end

for imgindex,imgparam in pairs(server.uploads) do

    --
    -- copy the file to a safe place
    --
    local success, tmpname = server.uuid62()
    if not success then
        send_error(500, "Couldn't generate uuid62!")
        return -1
    end
    local tmppath =  tmpdir .. tmpname
    local success, result = server.copyTmpfile(imgindex, tmppath)
    if not success then
        send_error(500, "Couldn't copy uploaded file: " .. result)
        return -1
    end


    --
    -- create a SipiImage, already resized to the thumbnail size
    --
    local success, myimg = SipiImage.new(tmppath, {size = config.thumb_size})
    if not success then
        send_error(500, "Couldn't create thumbnail: " .. myimg)
        return -1
    end

    local filename = imgparam["origname"]
    local mimetype = imgparam["mimetype"]

    local success, check = myimg:mimetype_consistency(mimetype, filename)
    if not success then
        send_error(500, "Couldn't check mimteype consistency: " .. check)
        return -1
    end

    --
    -- if check returns false, the user's input is invalid
    --

    if not check then
        send_error(400, MIMETYPES_INCONSISTENCY)
        return -1
    end

    --
    -- get the dimensions and print them
    --
    local success, dims = myimg:dims()
    if not success then
        send_error(500, "Couldn't get image dimensions: " .. dims)
        return -1
    end


    --
    -- write the thumbnail file
    --
    local thumbsdir = config.imgroot .. '/thumbs/'
    local success, exists = server.fs.exists(thumbsdir)
    if not success then
        send_error(500, "Internal server error")
        return -1
    end
    if not exists then
        local success, result = server.fs.mkdir(thumbsdir, 511)
        if not success then
            send_error(500, "Couldn't create thumbsdir: " .. result)
            return -1
        end
    end


    local thumbname = thumbsdir .. tmpname .. "_THUMB.jpg"
    local success, result = myimg:write(thumbname)
    if not success then
        send_error(500, "Couldn't create thumbnail: " .. result)
        return -1
    end

    answer = {
        nx_thumb = dims.nx,
        ny_thumb = dims.ny,
        mimetype_thumb = 'image/jpeg',
        preview_path = "http://" .. config.hostname .. ":" .. config.port .."/thumbs/" .. tmpname .. "_THUMB.jpg" .. "/full/full/0/default.jpg",
        filename = tmpname, -- make this a IIIF URL
        original_mimetype = mimetype,
        original_filename = filename,
        file_type = 'IMAGE'
    }

end

send_success(answer)
