print("-------PUT script------")

require "../model/database"

-- Find ID from the URI
local id
startPos, endPos = string.find(server.uri, "api/resources/")

if (startPos ~= nil) and (endPos ~= nil) then
    local num = tonumber(string.sub(server.uri, endPos+1, string.len(server.uri)))
    if (num ~= nil) then
        id = math.floor(num)
    end
end

-- id was not found it the uri
if (id == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    return
end

-- id does not exist in the database
if (readData(id) == nil) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(404)
    return
end

-- Replaces file
if (server.uploads ~= nil) then
    for pdfindex,pdfparam in pairs(server.uploads) do
        print(pdfparam["origname"])

        --    os.rename("bird.jpg", "vogel.jpg")
        --    res = os.remove("vogel 2.jpg")
    end
end

-- Get parameters
local element = {}
for key,value in pairs(server.post) do
    if (key == '"date"') then
        element["date"] = value
    elseif (key == '"title"') then
        element["title"] = value
    else
        print("fail")
    end
end

-- Updates data in database
updateData(id, element)

table = {}
table["data"] = readData(id)

-- Check if data has changed
if (table["data"] ~= nil) and (table["data"]["title"] == element["title"]) and (table["data"]["date"] == element["date"]) then
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
server.sendStatus(200)
server.print(jsonstr)
