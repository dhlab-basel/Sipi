print("---- PUT collections script ----")

require "./model/parameter"
require "./model/collection"

-- Gets ID from the url
local id = getID("^/api/collections/%d+$")

-- ID was not found it the url
if (id == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    return
end

-- Gets the data with the id
local data = readCol(id)

-- ID not in the database
if (data == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(404)
    return
end

-- Gets parameters
local parameters = getColParams(server.post)

local oldParentID = data["collection_id"]
local newParentID = parameters["collection_id"]

-- Check if newParentID exists
if (readCol(newParentID) == nil) then
    print("No parent ID")
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(404)
    return
end

-- Corrects the isLeaf of parent
if (newParentID ~= nil) and (newParentID ~= oldParentID) then
    -- Get all the sibling of the new collection
    local p1 = { "AND", "collection_id", "EQ", newParentID, nil }
    local params = { p1 }
    local siblings = readAllCol(params)

    -- Correct isLeaf of the new parent
    if (#siblings == 0) then
        local param = {}
        param["isLeaf"] = 0
        updateCol(newParentID, param)
    end

    -- Get all the sibling of the old collection
    local p2 = { "AND", "collection_id", "EQ", oldParentID, nil }
    local p3 = { "AND", "id", "!EQ", id, nil }
    local params2 = { p2, p3 }
    local siblings2 = readAllCol(params2)

    -- Correct isLeaf of the old parent
    if (#siblings == 0) then
        local param2 = {}
        param2["isLeaf"] = 1
        updateCol(oldParentID, param2)
    end
end

-- Updates data in database
updateCol(id, parameters)

-- Reads the data and will be added to the JSON
local table = {}
table["data"] = readCol(id)

local success, jsonstr = server.table_to_json(table)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

server.sendHeader('Content-type', 'application/json')
server.sendStatus(200)
server.print(jsonstr)