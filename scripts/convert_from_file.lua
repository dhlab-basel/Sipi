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

-- Knora GUI-case: Sipi has already saved the file that is supposed to be converted
-- the file was saved to: config.imgroot .. '/tmp/' (route make_thumbnail)

server.setBuffer()


originalFilename = server.post['originalfilename']
originalMimetype = server.post['originalmimetype']
filename = server.post['filename']


-- check if all the expected params are set
if originalFilename == nil or originalMimetype == nil or filename == nil then
    result = {
        status = 1,
        message = "Parameters not set correctly"
    }

    jsonstr = server.table_to_json(result)

    server.print(jsonstr)

    return
end

-- file with name given in param "filename" has been saved by make_thumbnail.lua beforehand
tmpdir = config.imgroot .. '/tmp/'
sourcePath = tmpdir .. filename

-- check if soure is readable
if not server.fs.is_readable(sourcePath) then
    result = {
        status = 1,
        message = "File " .. filename .. " is not readable."
    }

    jsonstr = server.table_to_json(result)

    server.print(jsonstr)

    return

end

-- all params are set
server.sendHeader("Content-Type", "application/json")

--
-- check if knora directory is available, if not, create it
--
knoraDir = config.imgroot .. '/knora/'
if  not server.fs.exists(knoraDir) then
    server.fs.mkdir(knoraDir, 511)
end

baseName = server.uuid62()

-- create full quality image (jp2)
fullImg = SipiImage.new(sourcePath)

check = fullImg:mimetype_consistency(originalMimetype, originalFilename)

-- if check returns false, the user's input is invalid
if not check then
    result = {
        status = 1,
        message = "Mimetypes and/or file extension are inconsistent."
    }

    jsonstr = server.table_to_json(result)

    server.print(jsonstr)

    return
end

fullImgName = baseName .. '.jpx'
fullDims = fullImg:dims()
fullImg:write(knoraDir .. fullImgName)

-- create thumbnail (jpg)
thumbImg = SipiImage.new(sourcePath, {size = config.thumb_size})
thumbImgName = baseName .. '.jpg'
thumbDims = thumbImg:dims()
thumbImg:write(knoraDir .. thumbImgName)

-- delete tmp and preview files
server.fs.unlink(sourcePath)
server.fs.unlink(config.imgroot .. '/thumbs/' .. filename .. "_THUMB.jpg")

result = {
    status = 0,
    mimetype_full = "image/jp2",
    filename_full = fullImgName,
    nx_full = fullDims.nx,
    ny_full = fullDims.ny,
    mimetype_thumb = "image/jpeg",
    filename_thumb = thumbImgName,
    nx_thumb = thumbDims.nx,
    ny_thumb = thumbDims.ny,
    original_mimetype = originalMimetype,
    original_filename = originalFilename,
    file_type = 'image'
}
--
jsonstr = server.table_to_json(result)


--for kk,vv in pairs(result) do
--    print(kk, " = ", vv)
--end

server.print(jsonstr)
--]]
