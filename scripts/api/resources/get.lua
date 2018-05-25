print("---- GET resources script ----")

require "./model/resource"
require "./model/parameter"
require "./model/file"


local table1 = {}
local resourcePattern = "api/resources$"

local uri = server.uri

local id = getIDofBinaryFile(uri)

if (id ~= nil) then

    local data = readRes(id)

    -- Data does not exist in the database
    if (data == nil) then
        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(404)
        return
    end

    local filename = data["filename"]
    local mimetype = data["mimetype"]

    if (filename ~= nil) and (mimetype ~=nil) then
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
else
    id = getIDfromURL(uri)
    if (id ~= nil) then
        print(uri .. " ==> has IDPattern with = " .. id)

        table1["data"] = readRes(id)

        if (table1["data"] ~= nil) then
            table1["status"] = "successful"
        else
            table1["status"] = "no data were found"
        end

    else
        if (string.match(uri, resourcePattern) ~= nil) then

            table1["data"] =  readAllRes({})

            if #table1["data"] > 0 then
                table1["status"] = "successful"
            else
                table1["status"] = "no data were found"
            end

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
