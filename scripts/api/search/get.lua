print("---- GET search script ----")

require "./model/resource"
require "./model/parameter"
require "./model/file"
require "./model/query"

function startSearching()
    local table1 = {}

    -- Evaluation of the parameters
    local parameters = {}
    if (server.get ~= nil) then

        for key,value in pairs(server.get) do
            print(key, value)
            local p1, p2, p3, p4, p5

            -- Evaluates the logical operator
            local log = string.match(key, "%[.*%]")
            print(log)
            if (log ~= nil) then
                local startPara, endPara = string.find(key, "%[.*%]")
                if (startPara ~= nil) and (endPara ~= nil) then
                    p2 = string.sub(key, endPara+1, #key)
                    if (log == "[AND]") or (log == "[and]") then
                        p1 = "AND"
                    elseif (log == "[OR]") or (log == "[or]") then
                        p1 = "OR"
                    else
                        server.sendHeader('Content-type', 'application/json')
                        server.sendStatus(400)
                        return
                    end
                else
                    server.sendHeader('Content-type', 'application/json')
                    server.sendStatus(500)
                    return
                end
            else
                p1 = "AND"
                p2 = key
            end

            -- Evaluates the comparison operator
            local comp = string.match(value, "%[.*%]")
            print(comp)
            if (comp ~= nil) then
                local startVal, endVal = string.find(value, "%[.*%]")
                if (startVal ~= nil) and (endVal ~= nil) then
                    p4 = string.sub(value, endVal+1, #value)
                else
                    server.sendHeader('Content-type', 'application/json')
                    server.sendStatus(500)
                    return
                end
            else
                p3 = "EQ"
                p4 = value
            end

            if (p2 == "date") then
                local i,j = string.find(value, ":")
                if (i ~= nil) and (j ~= nil) then
                    if (comp == nil) then
                        print("gültige url im date")
                        p3 = "BETWEEN"
                        p4 = string.sub(value, 1, j-1)
                        p5 = string.sub(value, j+1, #value)
                    else
                        server.sendHeader('Content-type', 'application/json')
                        server.sendStatus(400)
                        return
                    end
                else
                    if (comp ~= nil) then
                        if (comp == "[eq]") or (comp == "[EQ]") then
                            p3 = "EQ"
                        elseif (comp == "[!eq]") or (comp == "[!EQ]") then
                            p3 = "!EQ"
                        elseif (comp == "[null]") or (comp == "[NULL]") then
                            p3 = "NULL"
                        elseif (comp == "[!null]" ) or (comp == "[!NULL]") then
                            p3 = "!NULL"
                        elseif (comp == "[gt]" ) or (comp == "[GT]") then
                            p3 = "GT"
                        elseif (comp == "[gt_eq]" ) or (comp == "[GT_EQ]") then
                            p3 = "GT_EQ"
                        elseif (comp == "[lt]" ) or (comp == "[LT]") then
                            p3 = "LT"
                        elseif (comp == "[lt_eq]" ) or (comp == "[LT_EQ]") then
                            p3 = "LT_EQ"
                        else
                            server.sendHeader('Content-type', 'application/json')
                            server.sendStatus(400)
                            return
                        end
                    end
                end
            else
                if (comp ~= nil) then
                    if (comp == "[eq]") or (comp == "[EQ]") then
                        p3 = "EQ"
                    elseif (comp == "[!eq]") or (comp == "[!EQ]") then
                        p3 = "!EQ"
                    elseif (comp == "[like]") or (comp == "[LIKE]") then
                        p3 = "LIKE"
                    elseif (comp == "[!like]") or (comp == "[!LIKE]") then
                        p3 = "!LIKE"
                    elseif (comp == "[null]") or (comp == "[NULL]") then
                        p3 = "NULL"
                    elseif (comp == "[!null]" ) or (comp == "[!NULL]") then
                        p3 = "!NULL"
                    else
                        server.sendHeader('Content-type', 'application/json')
                        server.sendStatus(400)
                        return
                    end
                end
            end

            local parameter = { p1, p2, p3, p4, p5 }
            table.insert(parameters, parameter)

            for j,k in pairs(parameter) do
                print(j, k)
            end

        end
    end

    table1["data"] =  readAllRes(parameters)

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

local baseURL = "^/api/"
local uri = server.uri

local routes = {}
routes[baseURL .. "search$"] = startSearching

for route, func in pairs(routes) do
    if (string.match(uri, route) ~= nil) then
        func()
        return
    end
end

print(uri .. " ==> FAIL")
server.sendStatus(404)
