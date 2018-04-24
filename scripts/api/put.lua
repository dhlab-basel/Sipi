print("-------PUT script------")

function getIDQuery(id)
    return 'SELECT * FROM pdfObject WHERE id = "'.. id .. '"'
end

function doesExistsQuery(id)
    local db = sqlite("testDB/testData.db", "RW")
    local qry = db << getIDQuery(id)
    local row = qry()

    if row == nil then
        qry = ~qry
        db = ~db
        return false
    else
        qry = ~qry
        db = ~db
        return true
    end
end

function putQuery(id, title, date)
    return 'UPDATE pdfObject SET title="'.. title ..'", date="'.. date ..'" WHERE id= "'.. id .. '";'
end

-- Find ID from the URI
local id
startPos, endPos = string.find(server.uri, "api/resources/")

if (startPos ~= nil) and (endPos ~= nil) then
    local num = tonumber(string.sub(server.uri, endPos+1, string.len(server.uri)))
    if (num ~= nil) then
        id = math.floor(num)
    end
end


if id ~= nil then
    -- id was found in the uri
    if doesExistsQuery(id) then

        -- Replaces file
        if server.uploads ~= nil then
            for pdfindex,pdfparam in pairs(server.uploads) do
                print(pdfparam["origname"])

                --    os.rename("bird.jpg", "vogel.jpg")
                --    res = os.remove("vogel 2.jpg")
            end
        end

        -- Get parameters
        for key,value in pairs(server.post) do
            print(key, value)

        end

        -- Updates data in database
        local db = sqlite("testDB/testData.db", "RW")
        local qry = db << putQuery(id, "Sherlock Holmes " .. id, "2018")
        local row = qry()

        qry = ~qry
        db = ~db
    else
        -- id does not exist in the database
        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(404)
        return
    end
else
    -- id was not found it the uri
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(400)
    return
end

table = {}

element1 = {}
element1["id"] = "1"
element1["title"] = "Phantom of the Opera"
element1["date"] = "04.12.2006"

table["subjects"] = { element1 }
table["status"] = "successful"

local success, jsonstr = server.table_to_json(table)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

--print(jsonstr)

server.sendHeader('Content-type', 'application/json')
server.sendStatus(200)
server.print(jsonstr)
