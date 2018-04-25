print("-------GET script------")

function selectAllQuery()
    return 'SELECT * FROM pdfObject'
end

function selectIDQuery(id)
    return 'SELECT * FROM pdfObject WHERE id = "'.. id .. '"'
end

function selectParametersQuery(parameters)
    return 'SELECT * FROM pdfObject WHERE ' .. parameters
end

function noDataStatus()
    return "no data were found"
end

function successStatus()
    return "successful"
end

function hasFilePattern(url, filePattern)
    local i, j = string.find(url, filePattern)
    if (i~=nil) and (j ~= nil) then
        return string.match(url,"%d+")
    else
        return nil
    end
end

function hasIDPattern(url, idPattern)
    if (string.match(url, idPattern) ~= nil) then
        return string.match(url, "%d+")
    else
        return nil
    end
end

function getData(id, table1)
    local db = sqlite("testDB/testData.db", "RW")
    local qry = db << selectIDQuery(id)
    local row = qry()

    if row ~= nil then
        -- ACHTUNG DIES IST LUA SYNTAKTISCH FALSCH
        element = {}
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

    return table1;
end

function getAllData(parameters, table1)
    local db = sqlite("testDB/testData.db", "RW")
    local qry

    if (#parameters ~= 0) then
        qry = db << selectParametersQuery(table.concat(parameters, " AND "))
    else
        qry = db << selectAllQuery()
    end

    local row = qry()
    local elements = {}

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

    return table1;
end

local table1 = {}

local filePattern = "api/resources/%d+/file$"
local idPattern = "api/resources/%d+$"
local resourcePattern = "api/resources$"

local uri = server.uri

local id = hasFilePattern(uri, filePattern)
if (id ~= nil) then
    print(uri .. " ==> has FilePattern with = " .. id)
else
    id = hasIDPattern(uri, idPattern)
    if (id ~= nil) then
        print(uri .. " ==> has IDPattern with = " .. id)

        table1 = getData(id, table1)

    else
        if (string.match(uri, resourcePattern) ~= nil) then
            print(uri .. " ==> has ressourcePattern")

            -- Inserts all the parameters
            local parameters = {}
            if (server.get ~= nil) then
                for key,value in pairs(server.get) do
                    -- Equal search
                    -- table.insert(parameters, key ..'=' .. '"' .. value .. '"')
                    -- Like search
                    table.insert(parameters, key ..' like ' .. '"%' .. value .. '%"')
                end
            end

            table1 = getAllData(parameters, table1)

        else
            print(uri .. " ==> FAIL")

            server.sendStatus(404)
            return
        end
    end
end


server.setBuffer()

local success, jsonstr = server.table_to_json(table1)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

server.sendHeader('Content-type', 'application/json')
server.sendStatus(200)
server.print(jsonstr)
