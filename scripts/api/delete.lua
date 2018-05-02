print("-------DELETE script------")

require "../model/database"

function successStatus()
    return "successful"
end

flist = cache.filelist('AT_ASC')

table = {}

--local param, value = next(server.get, nil)

local uri = server.uri

-- Find ID from the URL
i, j = string.find(uri, "api/resources/")
local id
if (i ~= nil) and (j ~= nil) then
    i = tonumber(string.sub(uri, j+1, string.len(uri)))
    if (i ~= nil) and (type(i)) == "number" then
        id = i
    end
end

-- id was not found it the uri
if (id == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    return
end

-- id not in the database
if (readData(id) == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(500)
    return
end

deleteData(id)

-- id still exists and delete failed
if (readData(id) ~= nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(500)
    return
end

table["status"] = successStatus()

local success, jsonstr = server.table_to_json(table)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

server.sendHeader('Content-type', 'application/json')
server.sendStatus(200)
server.print(jsonstr)
