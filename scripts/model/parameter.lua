--
-- Created by IntelliJ IDEA.
-- User: vijeinath
-- Date: 03.05.18
-- Time: 13:12
-- To change this template use File | Settings | File Templates.
--

function getResParams(serverParam)
    local parameters = {}
    for key,value in pairs(serverParam) do
        if (key == 'title') then
            parameters["title"] = value
        elseif (key == 'creator') then
            parameters["creator"] = value
        elseif (key == 'subject') then
            parameters["subject"] = value
        elseif (key == 'description') then
            parameters["description"] = value
        elseif (key == 'publisher') then
            parameters["publisher"] = value
        elseif (key == 'contributor') then
            parameters["contributor"] = value
        elseif (key == 'date_start') then
            parameters["date_start"] = value
        elseif (key == 'date_end') then
            parameters["date_end"] = value
        elseif (key == 'format') then
            parameters["format"] = value
        elseif (key == 'identifier') then
            parameters["identifier"] = value
        elseif (key == 'source') then
            parameters["source"] = value
        elseif (key == 'language') then
            parameters["language"] = value
        elseif (key == 'relation') then
            parameters["relation"] = value
        elseif (key == 'coverage') then
            parameters["coverage"] = value
        elseif (key == 'rights') then
            parameters["rights"] = value
        elseif (key == 'collection_id') then
            parameters["collection_id"] = value
        else
            print("the following key does not match: " .. key)
        end
    end

    return parameters
end

function getColParams(serverParam)
    local parameters = {}
    for key,value in pairs(serverParam) do
        if (key == 'name') then
            parameters["name"] = value
        elseif (key == 'collection_id') then
            parameters["collection_id"] = value
        else
            print("the following key does not match: " .. key)
        end
    end

    return parameters
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
