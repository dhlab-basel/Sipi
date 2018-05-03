print("-------DELETE script------")

require "../model/database"
require "../model/parameter"

function successStatus()
    return "successful"
end

-- gets ID from the url
local id = getIDfromURL()

-- id was not found it the url
if (id == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    return
end

-- id not in the database
if (readData(id) == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(404)
    return
end

deleteData(id)

-- id still exists and delete failed
if (readData(id) ~= nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(500)
    return
end

local table = {}
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
