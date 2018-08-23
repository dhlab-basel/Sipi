print("---- POST resources script ----")

-- Required external script files
require "./model/resource"
require "./model/parameter"
require "./model/file"
require "./model/collection"

local uriPattern = "api/resources$"

-- Checks if url is correct
if (string.match(server.uri, uriPattern) == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    server.print(jsonstr)
    return
end

-- Gets parameters
local parameters, errMsg = getResParams(server.post)

-- Checks if parameters were given
if (errMsg ~= nil) then
    local table = {}
    table["data"] = { }
    table["status"] = "not all parameters given"

    local success, jsonstr = server.table_to_json(table)
    if not success then
        server.sendStatus(500)
        server.log(jsonstr, server.loglevel.err)
        return false
    end

    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(errMsg)
    server.print(jsonstr)
    return
end

-- Gets collection ID of resource
local col_id = parameters["collection_id"]

-- Checks if parameters "title" and "collection_id" are given
if ((col_id == nil) or (parameters["title"] == nil)) then
    local table = {}
    table["data"] = {}
    table["status"] = "parameter title and collection_id not given"

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

local collection = readCol(col_id)

-- Checks if collection exists
if (collection == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(404)
    return
end

-- Checks if collection is a Leaf
if (collection["isLeaf"] ~= 1) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(402)
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
