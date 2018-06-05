print("---- DELETE collections script ----")

require "./model/parameter"
require "./model/collection"
require "./model/resource"

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

-- Checks if collection is root
if (data["id"] == 1) then
    print("Is root collection")
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(403)
    return
end

-- Checks if collection is a leaf
if (data["isLeaf"] == 0) then
    print("Is a leaf")
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(403)
    return
end

-- Checks if there is any resources attached
local parameter = { "AND", "collection_id", "EQ", id, nil }
local resources = readAllRes({ parameter })

if (#resources > 0) then
    print("Resources attached")
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(403)
    return
end

-- Get all the sibling of the collection
local parentID = readCol(id)['collection_id']
local p1 = { "AND", "collection_id", "EQ", parentID, nil }
local p2 = { "AND", "id", "!EQ", id, nil }
local siblings = readAllCol({ p1, p2 })

deleteCol(id)

-- ID still exists and delete failed
if (readCol(id) ~= nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(500)
    return
end

-- Correct isLeaf of the parent
if (#siblings == 0) then
    local param = {}
    param["isLeaf"] = 1
    updateCol(parentID, param)
end

local table = {}
table["data"] = collections
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