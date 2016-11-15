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
print ("111")
    --
    -- check if tmporary directory is available, if not, create it
    --
    tmpdir = config.imgroot .. '/tmp/'
    if  not server.fs.exists(tmpdir) then
        local success, result = pcall (function(a,b)  server.fs.mkdir(a,b) end, tmpdir, 511)
        if not success then
            print("server.fs.mkdir: ", tmpdir, " ; ", result)
        end
    end

print("222")
    --
    -- copy the file to a safe place
    --
    tmpname = server.uuid62()
    tmppath =  tmpdir .. tmpname
    --server.copyTmpfile(imgindex, tmppath)
    local success, result = pcall (function(a,b)  server.copyTmpfile(a,b) end, imgindex, tmppath)
    if not success then
        print("server.copyTmpfile: ", result)
    end
print("333")
    --
    -- create a SipiImage, already resized to the thumbnail size
    --
    print ("tmppath=", tmppath)
    print ("config.thumb_size=", config.thumb_size)
    myimg = SipiImage.new(tmppath, {size = config.thumb_size})
    --local myimg, error = pcall (function(a,b) return SipiImage.new(a,b) end, tmppath, {size = config.thumb_size})
    --if not myimg then
    --   print("SipiImage.new: ", error)
    --end
print("444")

    filename = imgparam["origname"]
    mimetype = imgparam["mimetype"]

    --check = myimg:mimetype_consistency(mimetype, filename)
    local check, result = pcall (function(a,b)  return myimg:mimetype_consistency(a,b) end, mimetype, filename)
    if not check then
        print("myimg:mimetype_consistency : ", result)
    end
print("555")
    -- if check returns false, the user's input is invalid
    if not check then

        send_error(400, MIMETYPES_INCONSISTENCY)

        return
    end

    --
    -- get the dimensions and print them
    --
    dims = myimg:dims()
print("666")
    --
    -- write the thumbnail file
    --
    thumbsdir = config.imgroot .. '/thumbs/'
    if  not server.fs.exists(thumbsdir) then
        server.fs.mkdir(thumbsdir, 511)
    end
print("777")
    thumbname = thumbsdir .. tmpname .. "_THUMB.jpg"
    myimg:write(thumbname)
print("888")
    result = {
        nx_thumb = dims.nx,
        ny_thumb = dims.ny,
        mimetype_thumb = 'image/jpeg',
        preview_path = "http://localhost:1024/thumbs/" .. tmpname .. "_THUMB.jpg" .. "/full/full/0/default.jpg",
        filename = tmpname, -- make this a IIIF URL
        original_mimetype = mimetype,
        original_filename = filename,
        file_type = 'IMAGE'
    }
    gaga = { me = "Lukas", you = "Ben"}
    print("999")
    send_success(gaga)

    --server.sendHeader("Content-Type", "application/json")
    --print("999")
    --jsonstr = server.table_to_json(result)
    --print(jsonstr)
    --server.print(jsonstr)
    print("THE END")

end
