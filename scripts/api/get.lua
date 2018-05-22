print("-------GET script------")

require "../model/database"
require "../model/parameter"

function noDataStatus()
    return "no data were found"
end

function successStatus()
    return "successful"
end


local table1 = {}

local resourcePattern = "api/resources$"
local searchPattern = "api/resources?search="

local uri = server.uri

local id = getIDofBinaryFile(uri)

if (id ~= nil) then
    local filename = readData(id)["filename"]
    local mimetype = readData(id)["mimetype"]

    if (filename ~= nil) and (mimetype ~=nil) then
        local path = "data/tmp/" .. filename
        io.input(path)
        local s = io.read("*a")
        server.setBuffer()
        server.sendHeader('Content-type', mimetype)
        server.sendStatus(200)
        server.print(s)
        return
    end
else
    id = getIDfromURL(uri)
    if (id ~= nil) then
        print(uri .. " ==> has IDPattern with = " .. id)

        table1["data"] = readData(id)

        if (table1["data"] ~= nil) then
            table1["status"] = successStatus()
        else
            table1["status"] = noDataStatus()
        end

    else
        if (string.match(uri, resourcePattern) ~= nil) then
            print(uri .. " ==> has ressourcePattern")

            -- Inserts all the parameters
            local parameters = {}
            if (server.get ~= nil) then
                for key,value in pairs(server.get) do
                    local paramName
                    local a = string.match(key, "%[!?%a+_?%a+%]")
                    local startPos, endPos = string.find(key, "%[!?%a+_?%a+%]")
                    if (startPos ~= nil) and (endPos ~= nil) then
                        paramName = string.sub(key, 1, startPos-1)
                        print("found | " .. paramName)
                    else
                        paramName = key
                        print("not found | " .. paramName)
                    end
                    local i,j = string.find(value, ":")
                    if (paramName == "date") and (i ~= nil) and (j ~= nil) then
                        print(i, j)
                        if (a == nil) then
                            print("gültige url im date")
                        else
                            print("nicht gültig url im date")
                        end
                    else
                        if (a ~= nil) then
                            if (a == "[eq]") then
                                print("EQUAL")
                            elseif (a == "[!eq]" ) then
                                print("NOT EQUAL")
                            elseif (a == "[like]" ) then
                                print("LIKE")
                            elseif (a == "[!like]") then
                                print("NOT LIKE")
                            elseif (a == "[ex]" ) then
                                print("EXISTS")
                            elseif (a == "[!ex]" ) then
                                print("NOT EXISTS")
                            elseif (a == "[gt]" ) then
                                print("GREATER THAN")
                            elseif (a == "[gt_eq]" ) then
                                print("GREATER THAN EQUAL")
                            elseif (a == "[lt]" ) then
                                print("LESS THAN")
                            elseif (a == "[lt_eq]" ) then
                                print("LESS THAN EQUAL")
                            else
                                print("Ungültige []")
                            end
                        else
                            print("auch ungültig")
                        end
                    end

--                    if (key == "date") then
--                        local i,j = string.find(value, ":")
--                        if (i~= nil) and (j~= nil) then
--                            value = getDateRange(string.sub(value,1, i-1), string.sub(value,j+1, #value))
--                            table.insert(parameters, key ..' between '  .. value)
--                        else
--                            table.insert(parameters, key ..' like ' .. '"%' .. value .. '%"')
--                        end
--                    else
--                        table.insert(parameters, key ..' like ' .. '"%' .. value .. '%"')
--                    end
--                    -- Equal search
--                    -- table.insert(parameters, key ..'=' .. '"' .. value .. '"')
--                    -- Like search
                end
            end

            table1["data"] =  readAllData(parameters)

            if #table1["data"] > 0 then
                table1["status"] = successStatus()
            else
                table1["status"] = noDataStatus()
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
