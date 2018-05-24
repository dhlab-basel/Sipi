print("---- POST resources script ----")

require "./model/resources"
require "./model/parameter"
require "./model/file"

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

parameters = createFile(parameters)

local newID = createRes(parameters)

local table = {}
table["data"] = readRes(newID)

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
