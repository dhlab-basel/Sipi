print("---- GET collections script ----")

require "./model/collection"

baseURL = "^/api"
uri = server.uri

function getCollections()
    local table1 = {}
    local pattern1 = baseURL .. "/collections$"
    if (string.match(uri, pattern1) ~= nil) then
        print("gefunden")

        table1["data"] =  readAllCol()

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

        return
    end
end

function getCollection()
    local table1 = {}
    local pattern2 = baseURL .. "/collections/%d+$"
    if (string.match(uri, pattern2) ~= nil) then
        local id =  string.match(uri, "%d+")
        print("2. gefunden", id)

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
        return
    end
end

getCollections()

getCollection()

--Fall 4
print("FAIL - nichts gefunden")
server.sendStatus(404)