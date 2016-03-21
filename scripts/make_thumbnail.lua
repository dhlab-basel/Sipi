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

for imgindex,imgparam in pairs(server.uploads) do

    --
    -- print all upload parameters of the file
    --
    --print("List of Parameters...")
    --for kk,vv in pairs(imgparam) do
    --    print(kk, " = ", vv)
    --end

    server.sendHeader("Content-Type", "application/json")

    --
    -- check if tmporary directory is available, if not, create it
    --
    tmpdir = config.imgroot .. '/tmp/'
    if  not server.fs.exists(tmpdir) then
        server.fs.mkdir(tmpdir, 511)
    end

    --
    -- copy the file to a safe place
    --
    tmpname = server.uuid62()
    tmppath =  tmpdir .. tmpname
    server.copyTmpfile(imgindex, tmppath)

    --
    -- create a SipiImage, already resized to the thumbnail size
    --
    myimg = SipiImage.new(tmppath, {size = config.thumb_size})

    filename = imgparam["origname"]
    mimetype = imgparam["mimetype"]

    check = myimg:mimetype_consistency(mimetype, filename)

    -- if check returns false, the user's input is invalid
    if not check then
        result = {
            status = 1,
            message = "Mimetypes are inconsistent."
        }

        jsonstr = server.table_to_json(result)

        server.print(jsonstr)

        return
    end

    --
    -- get the dimensions and print them
    --
    dims = myimg:dims()

    --
    -- write the thumbnail file
    --
    thumbsdir = config.imgroot .. '/thumbs/'
    if  not server.fs.exists(thumbsdir) then
        server.fs.mkdir(thumbsdir, 511)
    end

    thumbname = thumbsdir .. tmpname .. "_THUMB.jpg"
    myimg:write(thumbname)

    result = {
        status = 0,
        nx_thumb = dims.nx,
        ny_thumb = dims.ny,
        mimetype_thumb = 'image/jpeg',
        preview_path = "http://localhost:1024/thumbs/" .. tmpname .. "_THUMB.jpg" .. "/full/full/0/default.jpg",
        filename = tmpname, -- make this a IIIF URL
        original_mimetype = mimetype,
        original_filename = filename,
        file_type = 'IMAGE'
    }
    jsonstr = server.table_to_json(result)

    server.print(jsonstr)

end
