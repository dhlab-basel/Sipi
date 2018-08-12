print("---- GET search script ----")

-- Required external script files
require "./model/resource"
require "./model/parameter"
require "./model/file"
require "./model/query"

-- Function definitions
function getSearchword(parameters)
    local found = 0
    local searchword, errMsg

    if (parameters ~= nil) then
        for key,value in pairs(parameters) do
            if (key == "searchword") and (found == 0)then
                searchword = value
                found = 1
            else
                errMsg = 404
            end
        end
    else
        searchword = ""
    end

    return searchword, errMsg
end

function fullSearch()
    local searchword, errMsg

    searchword, errMsg = getSearchword(server.get)

    if (errMsg ~= nil) then
        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(errMsg)
        return
    end

    local table1 = {}
    table1["data"] = readAllResFullText(searchword)

    if #table1["data"] > 0 then
        table1["status"] = "successful"
    else
        table1["status"] = "no data were found"
        table1["data"] = {{}}
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

function structureParam(value, paramName)
    local p2, p3, p4, errMsg
    local comp = string.match(value, "%[.*%]")

    if (comp ~= nil) then
        local startVal, endVal = string.find(value, "%[.*%]")
        if (startVal ~= nil) and (endVal ~= nil) then
            p3 = string.sub(value, endVal+1, #value)
        else
            errMsg = 500
        end
    else
        p2 = "EQ"
        p3 = value
    end

    if (paramName == "date") then
        local i,j = string.find(value, ":")
        if (i ~= nil) and (j ~= nil) then
            if (comp == nil) then
                p2 = "BETWEEN"
                p3 = string.sub(value, 1, j-1)
                p4 = string.sub(value, j+1, #value)
            else
                errMsg = 400
            end
        else
            if (comp ~= nil) then
                if (comp == "[eq]") or (comp == "[EQ]") then
                    p2 = "EQ"
                elseif (comp == "[like]") or (comp == "[LIKE]") then
                    p2 = "LIKE"
                elseif (comp == "[null]") or (comp == "[NULL]") then
                    p2 = "NULL"
                elseif (comp == "[!null]" ) or (comp == "[!NULL]") then
                    p2 = "!NULL"
                elseif (comp == "[gt]" ) or (comp == "[GT]") then
                    p2 = "GT"
                elseif (comp == "[gt_eq]" ) or (comp == "[GT_EQ]") then
                    p2 = "GT_EQ"
                elseif (comp == "[lt]" ) or (comp == "[LT]") then
                    p2 = "LT"
                elseif (comp == "[lt_eq]" ) or (comp == "[LT_EQ]") then
                    p2 = "LT_EQ"
                else
                    errMsg = 400
                end
            end
        end
    else
        if (comp ~= nil) then
            if (comp == "[eq]") or (comp == "[EQ]") then
                p2 = "EQ"
            elseif (comp == "[!eq]") or (comp == "[!EQ]") then
                p2 = "!EQ"
            elseif (comp == "[like]") or (comp == "[LIKE]") then
                p2 = "LIKE"
            elseif (comp == "[!like]") or (comp == "[!LIKE]") then
                p2 = "!LIKE"
            elseif (comp == "[null]") or (comp == "[NULL]") then
                p2 = "NULL"
            elseif (comp == "[!null]" ) or (comp == "[!NULL]") then
                p2 = "!NULL"
            else
                errMsg = 400
            end
        end
    end

    return paramName, p2, p3, p4, errMsg
end

function extendedSearch()

    local parameters = {}
    if (server.get ~= nil) then

        for key,value in pairs(server.get) do

            -- Hier prüfen of key richtig ist und keine Sonderzeichen enthält

            -- Evaluates the parameters
            local p1, p2, p3, p4, errMsg = structureParam(value, key)

            if (errMsg ~= nil) then
                server.sendHeader('Content-type', 'application/json')
                server.sendStatus(errMsg)
                return
            end

            local parameter = { p1, p2, p3, p4 }
            table.insert(parameters, parameter)

            for j,k in pairs(parameter) do
                print(j, k)
            end
        end
    end

    local table1 = {}
    table1["data"] =  readAllRes(parameters)

    if #table1["data"] > 0 then
        table1["status"] = "successful"
    else
        table1["status"] = "no data were found"
        table1["data"] = {{}}
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

-- Checking of the url and appling the appropriate function
local baseURL = "^/api/"
local uri = server.uri

local routes = {}
routes[baseURL .. "search$"] = fullSearch
routes[baseURL .. "search/full$"] = fullSearch
routes[baseURL .. "search/extended$"] = extendedSearch

for route, func in pairs(routes) do
    if (string.match(uri, route) ~= nil) then
        func()
        return
    end
end

-- Fails if no url matches
server.sendStatus(404)
