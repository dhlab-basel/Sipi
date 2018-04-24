print("-------GET script------")

function getAllQuery()
    return 'SELECT * FROM pdfObject'
end

function getIDQuery(id)
    return 'SELECT * FROM pdfObject WHERE id = "'.. id .. '"'
end

function getParametersQuery(parameters)
    return 'SELECT * FROM pdfObject WHERE ' .. parameters
end

function noDataStatus()
    return "no data were found"
end

function successStatus()
    return "successful"
end

table1 = {}
element = {}
elements = {}

local uri = server.uri

-- Find ID from the URL
i, j = string.find(uri, "api/resources/")
local id
if (i ~= nil) and (j ~= nil) then
    i = tonumber(string.sub(uri, j+1, string.len(uri)))
    if (i ~= nil) and (type(i)) == "number" then
        id = i
    end
end

-- Gets data with the given ID
if id ~= nil then
    local db = sqlite("testDB/testData.db", "RW")
    local qry = db << getIDQuery(id)
    local row = qry()

    if row ~= nil then
        -- ACHTUNG DIES IST LUA SYNTAKTISCH FALSCH
        element["id"] = row[0]
        element["title"] = row[1]
        element["date"] = row[2]
        table1["data"]  = element
        table1["status"] = successStatus()
    else
        table1["data"]  = { element }
        table1["status"] = noDataStatus()
    end

    qry = ~qry -- delete query and free prepared statment
    db = ~db -- delete the database connection

-- Gets all the data
else

    -- Inserts all the parameters
    local parameters = {}
    if (server.get ~= nil) then
        for key,value in pairs(server.get) do
            table.insert(parameters, key ..'=' .. '"' .. value .. '"')
        end
    end

    local db = sqlite("testDB/testData.db", "RW")
    local qry

    if (#parameters ~= 0) then
        qry = db << getParametersQuery(table.concat(parameters, " AND "))
    else
        qry = db << getAllQuery()
    end

    local row = qry()

    while (row) do
        table.insert(elements, {["id"] = row[0],["title"]= row[1],["date"]=row[2]})
        row = qry()
    end

    table1["data"] =  elements

    if #elements > 0 then
        table1["status"] = successStatus()
    else
        table1["status"] = noDataStatus()
    end

    qry = ~qry -- delete query and free prepared statment
    db = ~db -- delete the database connection
end


server.setBuffer()

local success, jsonstr = server.table_to_json(table1)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

local success, result = server.http("GET", "http://localhost:1024/test3/lena512.tif/full/full/0/default.jpg");

--server.sendHeader('Content-type', 'image/jpeg')
--server.sendStatus(200)
--server.print(result.body)

server.sendHeader('Content-type', 'application/json')
server.sendStatus(200)
server.print(jsonstr)

