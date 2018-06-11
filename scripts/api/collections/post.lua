print("---- POST collections script ----")

-- Required external script files
require "./model/parameter"
require "./model/collection"

local uriPattern = "api/collections$"

-- Checks if url is correct
if (string.match(server.uri, uriPattern) == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    server.print(jsonstr)
    return
end

-- Gets parameters
local parameters = getColParams(server.post)
local parent = parameters["collection_id"]

-- Check if parent exists
if (readCol(parent) == nil) then
    print("Parent ID does not exist")
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(404)
    return
end

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

local newID = createCol(parameters)

-- Corrects isLeaf of parent
local parent = readCol(parameters['collection_id'])
if (parent['isLeaf'] == 1) then
    local params = {}
    params['isLeaf'] = 0
    updateCol(parent['id'], params)
end

local table = {}
table["data"] = readCol(newID)

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
