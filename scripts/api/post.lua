print("-------POST script------")

require "../model/database"
require "../model/parameter"

local uriPattern = "api/resources$"

-- Checks if url is correct
if (string.match(server.uri, uriPattern) == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    server.print(jsonstr)
    return
end

-- Gets parameters
local parameters = getParameters()

-- Checks if parameters were given
if (parameters == nil) then
    local table = {}
    table["data"] = { }
    table["status"] = "no parameter given"

    local success, jsonstr = server.table_to_json(table)
    if not success then
        server.sendStatus(500)
        server.log(jsonstr, server.loglevel.err)
        return false
    end

    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    server.print(jsonstr)
    return
end

-- Checks if a file was attached
if (server.uploads == nil) then
    local table = {}
    table["data"] = {}
    table["status"] = "no file attached"

    local success, jsonstr = server.table_to_json(table)
    if not success then
        server.sendStatus(500)
        server.log(jsonstr, server.loglevel.err)
        return false
    end

    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    server.print(jsonstr)
    return
end

for fileIndex, fileParam in pairs(server.uploads) do

    if (fileParam["filesize"] > 600000) then

        local table = {}
        table["data"] = { }
        table["status"] = "file too big"

        local success, jsonstr = server.table_to_json(table)
        if not success then
            server.sendStatus(500)
            server.log(jsonstr, server.loglevel.err)
            return false
        end

        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(413)
        server.print(jsonstr)
        return

    end

    print(fileParam["origname"])
    -- Copy file (only on unix systems)
--    os.execute("cp vogel.jpg kopie.jpg");

    local startPos, endPos = string.find(fileParam["origname"], "%.")
    local fileEnding = string.sub(fileParam["origname"], endPos+1, string.len(server.uri))

    local tmpdir = 'data/tmp/'
    local success, exists = server.fs.exists(tmpdir)

    if not success then
        -- Was ist das für ein Fall?
    end

    if not exists then
        local success, errmsg = server.fs.mkdir(tmpdir, 511)
        if not success then
            --            server.print("<br> Directory couldn't be created <br>")
        end
    else
        --        server.print("directory exists <br>")
    end

    local success, uuid62 = server.uuid62()
    if not success then
    end

    parameters["filename"] = uuid62
    parameters["fileextension"] = fileEnding
    parameters["mimetype"] = fileParam["mimetype"]
    parameters["filesize"] = fileParam["filesize"]

    local tmppath =  tmpdir .. parameters["filename"] .. '.' .. parameters["fileextension"]

    local success, errmsg = server.copyTmpfile(fileIndex, tmppath)
    if not success then
    else
    end

end

local newID = createData(parameters)

local table = {}
table["data"] = readData(newID)

-- Tests if data was created in the database
if (table["data"] ~= nil) and (type(table["data"]["id"]) == "number") then
    table["status"] = "successful"
else
    table["status"] = "unsuccessful"
end

local success, jsonstr = server.table_to_json(table)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

server.sendHeader('Content-type', 'application/json')
server.sendStatus(201)
server.print(jsonstr)
