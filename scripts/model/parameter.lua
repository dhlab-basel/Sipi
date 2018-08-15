--
-- Created by IntelliJ IDEA.
-- User: vijeinath
-- Date: 03.05.18
-- Time: 13:12
-- To change this template use File | Settings | File Templates.
--

function getResParams(serverParam)
    local parameters = {}
    local errMsg

    local resParamList = {
        "title",
        "creator",
        "subject",
        "description",
        "publisher",
        "contributor",
        "date_start",
        "date_end",
        "format",
        "identifier",
        "source",
        "language",
        "relation",
        "coverage",
        "rights",
        "collection_id"
    }

    -- Checks if all parameters are in the serverParams and allocates them to data structure
    for key, value in pairs(resParamList) do
        if (serverParam[value] ~= nil) then
            parameters[value] = serverParam[value]
        else
            print("parameter is missing: " .. value)
            errMsg = 400
        end
    end

    return parameters, errMsg
end

function getColParams(serverParam)
    local parameters = {}
    local errMsg

    local colParamList = {
        "name",
        "collection_id"
    }

    for key,value in pairs(colParamList) do
        if (serverParam[value] ~= nil) then
            parameters[value] = serverParam[value]
        else
            print("parameter is missing: " .. value)
            errMsg = 400
        end
    end

    return parameters, errMsg
end

--function getIDfromURL()
--    local id
--    local startPos, endPos = string.find(server.uri, "api/resources/")
--
--    if (startPos ~= nil) and (endPos ~= nil) then
--        local num = tonumber(string.sub(server.uri, endPos+1, string.len(server.uri)))
--        if (num ~= nil) then
--            id = math.floor(num)
--        end
--    end
--
--    return id
--end

function getID(pattern)
    if (string.match(server.uri, pattern) ~= nil) then
        return string.match(server.uri, "%d+")
    else
        return nil
    end
end

function getIDofBinaryFile(url)
    local filePattern = "^/api/resources/%d+/file$"
    local i, j = string.find(url, filePattern)
    if (i~=nil) and (j ~= nil) then
        return string.match(url,"%d+")
    else
        return nil
    end
end
