print("---- GET collections script ----")

-- Required external script files
require "./model/collection"
require "./model/resource"
require "./model/file"

-- Function definitions
function getCollections()
    local table1 = {}
    table1["data"] = readAllCol({})

    if (#table1["data"] > 0) then
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

function getCollection()
    local table1 = {}
    local id = string.match(uri, "%d+")
    table1["data"] = readCol(id)

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

function getResources()
    local table1 = {}
    local colID = string.match(uri, "%d+")

    local parameter = { "collection_id", "EQ", colID, nil }
    local parameters = { parameter }
    table1["data"] = readAllRes(parameters)

    if (#table1["data"] > 0) then
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
    local colID
    local resID

    local i, j = string.find(uri, "/collections/%d+")
    if (i ~= nil) and (j ~= nil) then
        colID = string.match(string.sub(uri, i, j), "%d+")
    end

    local k, l = string.find(uri, "/resources/%d+")
    if (k ~= nil) and (l ~= nil) then
        resID = string.match(string.sub(uri, k, l), "%d+")
    end

    local p1 = { "collection_id", "EQ", colID, nil }
    local p2 = { "id", "EQ", resID, nil }
    local parameters = { p1, p2 }

    local table1 = {}
    table1["data"] = readAllRes(parameters)[1]

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
    local colID
    local resID

    local i, j = string.find(uri, "/collections/%d+")
    if (i ~= nil) and (j ~= nil) then
        colID = string.match(string.sub(uri, i, j), "%d+")
    end

    local k, l = string.find(uri, "/resources/%d+")
    if (k ~= nil) and (l ~= nil) then
        resID = string.match(string.sub(uri, k, l), "%d+")
    end

    local p1 = { "collection_id", "EQ", colID, nil }
    local p2 = { "id", "EQ", resID, nil }
    local parameters = { p1, p2 }

    local data = readAllRes(parameters)[1]

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

-- Checking of the url and appling the appropriate function
baseURL = "^/api"
uri = server.uri

routes = {}
routes[baseURL .. "/collections$"] = getCollections
routes[baseURL .. "/collections/%d+$"] = getCollection
routes[baseURL .. "/collections/%d+/resources$"] = getResources
routes[baseURL .. "/collections/%d+/resources/%d+$"] = getResource
routes[baseURL .. "/collections/%d+/resources/%d+/file$"] = getFileResource

for route, func in pairs(routes) do
    if (string.match(uri, route) ~= nil) then
        func()
        return
    end
end

-- Fails if no url matches
server.sendStatus(404)