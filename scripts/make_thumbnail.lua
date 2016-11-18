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

server.setBuffer()

--
-- check if tmporary directory is available, if not, create it
--
local tmpdir = config.imgroot .. '/tmp/'
if  not server.fs.exists(tmpdir) then
    local success, result = pcall (server.fs.mkdir, tmpdir, 511)
    if not success then
        send_error(500, "Couldn't create tmpdir: " .. result)
        return
    end
end


for imgindex,imgparam in pairs(server.uploads) do

    --
    -- print all upload parameters of the file
    --
    --print("List of Parameters...")
    --for kk,vv in pairs(imgparam) do
    --    print(kk, " = ", vv)
    --end


    --
    -- copy the file to a safe place
    --
    local tmpname = server.uuid62()
    local tmppath =  tmpdir .. tmpname
    --server.copyTmpfile(imgindex, tmppath)
    local success, result = pcall(server.copyTmpfile, imgindex, tmppath)
    if not success then
        send_error(500, "Couldn't copy uploaded file: " .. result)
        return
    end


    --
    -- create a SipiImage, already resized to the thumbnail size
    --
    local success, myimg = SipiImage.new(tmppath, {size = config.thumb_size})
    if not success then
        send_error(500, "Couldn't create thumbnail: " .. myimg)
        return
    end

    local filename = imgparam["origname"]
    local mimetype = imgparam["mimetype"]

    local success, check = myimg:mimetype_consistency(mimetype, filename)
    if not success then
        send_error(500, "Couldn't check mimteype consistency: " .. check)
        return
    end

    --
    -- if check returns false, the user's input is invalid
    --

    if not check then
        send_error(400, MIMETYPES_INCONSISTENCY)
        return
    end

    --
    -- get the dimensions and print them
    --
    local success, dims = myimg:dims()
    if not success then
        send_error(500, "Couldn't get image dimensions: " .. dims)
        return
    end


    --
    -- write the thumbnail file
    --
    local thumbsdir = config.imgroot .. '/thumbs/'
    if  not server.fs.exists(thumbsdir) then
        local success, result = pcall(server.fs.mkdir, thumbsdir, 511)
        if not success then
            send_error(500, "Couldn't create thumbsdir: " .. result)
            return
        end
    end


    local thumbname = thumbsdir .. tmpname .. "_THUMB.jpg"
    local success, result = myimg:write(thumbname)
    if not success then
        send_error(500, "Couldn't create thumbnail: " .. result)
        return
    end

    answer = {
        nx_thumb = dims.nx,
        ny_thumb = dims.ny,
        mimetype_thumb = 'image/jpeg',
        preview_path = "http://localhost:1024/thumbs/" .. tmpname .. "_THUMB.jpg" .. "/full/full/0/default.jpg",
        filename = tmpname, -- make this a IIIF URL
        original_mimetype = mimetype,
        original_filename = filename,
        file_type = 'IMAGE'
    }

end

send_success(answer)
