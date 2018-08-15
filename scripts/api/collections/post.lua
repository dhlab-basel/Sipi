print("---- POST collections script ----")

-- Required external script files
require "./model/parameter"
require "./model/collection"
require "./model/resource"

local uriPattern = "api/collections$"

-- Checks if url is correct
if (string.match(server.uri, uriPattern) == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    server.print(jsonstr)
    return
end

local parameters, errMsg = getColParams(server.post)

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

-- Get parent collection
local parent = readCol(parameters['collection_id'])

-- Check if parent exists
if (parent == nil) then
    print("Parent ID does not exist")
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(404)
    return
end

local p1 = { "collection_id", "EQ", parameters['collection_id'], nil }
local resChildren = readAllRes({ p1 })

if (#resChildren > 0) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(403)
    return
end


local newID = createCol(parameters)

-- Corrects isLeaf of parent
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
