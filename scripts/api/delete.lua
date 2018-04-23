print("-------DELETE script------")

function getQuery(id)
    return 'DELETE FROM pdfObject WHERE id = "'.. id .. '"'
end

function noDataStatus()
    return "no data found"
end

function successStatus()
    return "successful"
end

flist = cache.filelist('AT_ASC')

table1 = {}
element = {}

--local param, value = next(server.get, nil)

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

print(id)

if id ~= nil then
    local db = sqlite("testDB/testData.db", "RW")
    local qry = db << getQuery(id)
    local row = qry()


    qry = ~qry -- delete query and free prepared statment
    db = ~db -- delete the database connection

    --table1["status"] = "invalid id"
end

table1["data"]  = { element }

local success, jsonstr = server.table_to_json(table1)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

server.sendHeader('Content-type', 'application/json')
server.sendStatus(200)
server.print(jsonstr)
