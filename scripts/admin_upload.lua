--
-- Created by IntelliJ IDEA.
-- User: ak
-- Date: 05.12.17
-- Time: 15:27
-- To change this template use File | Settings | File Templates.
--
require "send_response"

success, errormsg = server.setBuffer()
if not success then
    return -1
end

files = {}
newfilename = {}

for findex,fparam in pairs(server.uploads) do

    --
    -- check if admin directory is available under server root, if not, create it
    --
    admindir = config.docroot .. '/admin/'
    local success, exists = server.fs.exists(admindir)
    if not success then
        server.sendStatus(500)
        server.log(exists, server.loglevel.error)
        return false
    end
    if not exists then
        local success, errmsg = server.fs.mkdir(admindir, 511)
        if not success then
            server.sendStatus(500)
            server.log(errmsg, server.loglevel.error)
            return false
        end
    end

    --
    -- copy the file to admin directory
    --

    success, uuid62 = server.uuid62()
    origname = fparam["origname"]:gsub("%s+", "-")

    adminpath =  admindir .. uuid62 .. '-' .. origname
    local success, errmsg = server.copyTmpfile(findex, adminpath)
    if not success then
        server.sendStatus(500)
        server.log(errmsg, server.loglevel.error)
        return false
    else
        files[findex] = uuid62 .. '-' .. origname
    end
end

answer = {
    status = "OK",
    files = files
}

send_success(answer)
