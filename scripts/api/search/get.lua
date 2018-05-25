print("---- GET search script ----")

-- dient als hilfe
require "./model/resource"
require "./model/parameter"
require "./model/file"


local table1 = {}
local baseURL = "^/api/"
local searchPattern = baseURL .. "search"

local uri = server.uri
if (string.match(uri, searchPattern) ~= nil) then
     -- Inserts all the parameters
    local parameters = {}
    if (server.get ~= nil) then
        for key,value in pairs(server.get) do
            print(key, value)
            local paramName, valueName

            local log = string.match(key, "%[%a+%]" )
            local startPara, endPara = string.find(key, "%[%a+%]" )
            if (startPara ~= nil) and (endPara ~= nil) then
                paramName = string.sub(key, endPara+1, #key)
                print("found before " .. paramName)
            else
                paramName = key
                print("not found before " .. paramName)
            end

            local comp = string.match(value, "%[!?%a+_?%a+%]")
            local startVal, endVal = string.find(value, "%[!?%a+_?%a+%]")
            if (startVal ~= nil) and (endVal ~= nil) then
                valueName = string.sub(value, endVal+1, #value)
                print("found before " .. valueName)
            else
                valueName = value
                print("not found before " .. valueName)
            end

            local i,j = string.find(value, ":")
            if (paramName == "date") and (i ~= nil) and (j ~= nil) then
                print(i, j)
                if (comp == nil) then
                    print("gültige url im date")
                else
                    print("nicht gültig url im date")
                end
            else
                if (comp ~= nil) then
                    if (comp == "[eq]") or (comp == "[EQ]") then
                        print("EQUAL")
                    elseif (comp == "[!eq]") or (comp == "[!EQ]") then
                        print("NOT EQUAL")
                    elseif (comp == "[like]") or (comp == "[LIKE]") then
                        print("LIKE")
                    elseif (comp == "[!like]") or (comp == "[!LIKE]") then
                        print("NOT LIKE")
                    elseif (comp == "[ex]") or (comp == "[EX]") then
                        print("EXISTS")
                    elseif (comp == "[!ex]" ) or (comp == "[!EX]") then
                        print("NOT EXISTS")
                    elseif (comp == "[gt]" ) or (comp == "[GT]") then
                        print("GREATER THAN")
                    elseif (comp == "[gt_eq]" ) or (comp == "[GT_EQ]") then
                        print("GREATER THAN EQUAL")
                    elseif (comp == "[lt]" ) or (comp == "[LT]") then
                        print("LESS THAN")
                    elseif (comp == "[lt_eq]" ) or (comp == "[LT_EQ]") then
                        print("LESS THAN EQUAL")
                    else
                        print("Ungültige []")
                    end
                else
                    print("auch ungültig")
                end
            end
        end
    end

    table1["data"] =  readAllRes(parameters)

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
