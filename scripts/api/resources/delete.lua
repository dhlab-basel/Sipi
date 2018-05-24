print("---- DELETE resources script ----")

require "./model/database"
require "./api/model/parameter"
require "./api/model/file"

-- gets ID from the url
local id = getIDfromURL(server.uri)

-- id was not found it the url
if (id == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    return
end

-- gets the data with the id
local data = readData(id)

-- id not in the database
if (data == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(404)
    return
end

-- deletes file and metadata
deleteFile(data["filename"])
deleteData(id)

-- id still exists and delete failed
if (readData(id) ~= nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(500)
    return
end

local table = {}
table["status"] = "successful"

local success, jsonstr = server.table_to_json(table)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

server.sendHeader('Content-type', 'application/json')
server.sendStatus(200)
server.print(jsonstr)
