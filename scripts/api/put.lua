print("-------PUT script------")

require "../model/database"
require "../model/parameter"

-- Gets the ID
local id = getIDfromURL(server.uri)

-- ID was not found it the uri
if (id == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    return
end

-- ID does not exist in the database
if (readData(id) == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(404)
    return
end

-- Replaces file
if (server.uploads ~= nil) then
    for fileIndex, fileParam in pairs(server.uploads) do
        print(fileParam["origname"])

        --    os.rename("bird.jpg", "vogel.jpg")
        --    res = os.remove("vogel 2.jpg")
    end
end

-- Get parameters
local parameters = getParameters()

-- Updates data in database
updateData(id, parameters)

-- Reads the data and will be added to the JSON
local table = {}
table["data"] = readData(id)

local success, jsonstr = server.table_to_json(table)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

server.sendHeader('Content-type', 'application/json')
server.sendStatus(200)
server.print(jsonstr)
