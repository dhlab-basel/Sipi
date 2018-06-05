print("---- GET resources script ----")

require "./model/resource"
require "./model/parameter"
require "./model/file"

baseURL = "^/api"
uri = server.uri

function getResources()
    local table1 = {}
    print("gefunden")

    table1["data"] = readAllRes({})

    if #table1["data"] > 0 then
        table1["status"] = "successful"
    else
        table1["status"] = "no data were found"
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
end

function getResource()
    local table1 = {}
    local id = string.match(uri, "%d+")
    print("2. gefunden", id)

    table1["data"] = readRes(id)

    if (table1["data"] ~= nil) then
        table1["status"] = "successful"
    else
        table1["status"] = "no data were found"
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
end

function getFileResource()
    local id = string.match(uri, "%d+")
    print("3. gefunden", id)
    local data = readRes(id)

    -- Data does not exist in the database
    if (data == nil) then
        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(404)
        return
    end

    local filename = data["filename"]
    local mimetype = data["mimetype"]

    if (filename ~= nil) and (mimetype ~= nil) then
        local fileContent = readFile(filename)
        if (fileContent ~= nil) then
            server.setBuffer()
            server.sendHeader('Content-type', mimetype)
            server.sendStatus(200)
            server.print(fileContent)
            return
        else
            print("failed to read file")
        end
    end
end

routes = {}
routes[baseURL .. "/resources$"] = getResources
routes[baseURL .. "/resources/%d+$"] = getResource
routes[baseURL .. "/resources/%d+/file$"] = getFileResource

for route, func in pairs(routes) do
    if (string.match(uri, route) ~= nil) then
        func()
        return
    end
end

--Fail case
print("FAIL - nichts gefunden")
server.sendStatus(404)
