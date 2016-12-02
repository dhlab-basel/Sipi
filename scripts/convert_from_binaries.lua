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

-- handles the Knora non GUI-case: Knora uploaded a file to sourcePath

require "send_response"

success, errmsg = server.setBuffer()
if not success then
    server.log("server.setBuffer() failed: " .. errmsg, server.loglevel.error)
    return
end

originalFilename = server.post['originalfilename']
originalMimetype = server.post['originalmimetype']
sourcePath = server.post['source']

-- check if all the expected params are set
if originalFilename == nil or originalMimetype == nil or sourcePath == nil then

    send_error(400, PARAMETERS_INCORRECT)

    return
end

-- all params are set

--
-- check if knora directory is available, if not, create it
--
knoraDir = config.imgroot .. '/knora/'
success, exists = server.fs.exists(knoraDir)
if not success then
    server.log("server.fs.exists() failed: " .. exists, server.loglevel.error)
    return
end
if  not exists then
    success, errmsg = server.fs.mkdir(knoraDir, 511)
    if not success then
        server.log("server.fs.mkdir() failed: " .. errmsg, server.loglevel.error)
        return
    end
end

success, baseName = server.uuid62()
if not success then
    server.log("server.uuid62() failed: " .. baseName, server.loglevel.error)
    return
end

-- check if source is readable

success, readable = server.fs.is_readable(sourcePath)
if not success then
    server.log("server.fs.is_readable() failed: " .. readable, server.loglevel.error)
    return
end
if not readable then

    send_error(500, FILE_NOT_READBLE .. sourcePath)

    return

end

-- create full quality image (jp2)
success, fullImg = SipiImage.new(sourcePath)
if not success then
    server.log("SipiImage.new() failed: " .. fullImg, server.loglevel.error)
    return
end

success, check = fullImg:mimetype_consistency(originalMimetype, originalFilename)
if not success then
    server.log("fullImg:mimetype_consistency() failed: " .. check, server.loglevel.error)
    return
end

-- if check returns false, the user's input is invalid
if not check then

    send_error(400, MIMETYPES_INCONSISTENCY)

    return
end

fullImgName = baseName .. '.jpx'
success, fullDims = fullImg:dims()
if not success then
    server.log("fullImg:dims() failed: " .. fullDIms, server.loglevel.error)
    return
end

success, errmsg = fullImg:write(knoraDir .. fullImgName)
if not success then
    server.log("fullImg:write() failed: " .. errmsg, server.loglevel.error)
    return
end

-- create thumbnail (jpg)
success, thumbImg = SipiImage.new(sourcePath, {size = config.thumb_size})
if not success then
    server.log("SipiImage.new failed: " .. thumbImg, server.loglevel.error)
    return
end

thumbImgName = baseName .. '.jpg'

success, thumbDims = thumbImg:dims()
if not success then
    server.log("thumbImg:dims() failed: " .. thumbDims, server.loglevel.error)
    return
end


success, errmsg = thumbImg:write(knoraDir .. thumbImgName)
if not success then
    server.log("thumbImg:write failed: " .. errmsg, server.loglevel.error)
    return
end

result = {
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

send_success(result)
