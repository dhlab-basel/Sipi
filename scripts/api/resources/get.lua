print("---- GET resources script ----")

-- Required external script files
require "./model/resource"
require "./model/parameter"
require "./model/file"

function getXMLContent(data)
    local content = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" .. "\n" .. "<metadata>" .. "\n"
    local paramList = {
        "title",
        "creator",
        "subject",
        "description",
        "publisher",
        "contributor",
        "date",
        "format",
        "identifier",
        "source",
        "language",
        "relation",
        "coverage",
        "rights"
    }

    local temp
    for key, value in pairs(paramList) do
        if (value == "date") then
            if ((data["date_start"] ~= nil) and (data["date_end"] ~= nil)) then
                print(type(data["date_end"]))
                if ((data["date_start"] == data["date_end"])) then
                    temp = content .. "\t" .. "<dc:".. value .. ">" .. data["date_start"]  .. "</dc:" .. value .. ">" .. "\n"
                else
                    temp = content .. "\t" .. "<dc:".. value .. ">start=" .. data["date_start"]  .. ";end=".. data["date_end"] .. "</dc:" .. value .. ">" .. "\n"
                end
            else
                temp = content .. "\t" .. "<dc:".. value .. "></dc:" .. value .. ">" .. "\n"
            end
        elseif (data[value] ~= nil) then
            temp = content .. "\t" .. "<dc:".. value .. ">" .. data[value]  .. "</dc:" .. value .. ">" .. "\n"
        else
            temp = content .. "\t" .. "<dc:".. value .. "></dc:" .. value .. ">" .. "\n"
        end
        content = temp
    end

    return content .. "\b" .. "</metadata>"
end

-- Function definitions
function getResources()
    local table1 = {}

    table1["data"] = readAllRes({})

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

function getResource()
    local table1 = {}
    local id = string.match(uri, "%d+")

    table1["data"] = readRes(id)

    if (table1["data"] == nil) then
        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(404)
        print("no data was found")
        return
    else
        table1["status"] = "successful"
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
    local data = readRes(id)

    -- Data does not exist in the database
    if (data == nil) then
        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(404)
        print("no resource found in database")
        return
    end

    local serverFilename = data["filename"]
    local mimetype = data["mimetype"]

    if (serverFilename == nil) or (mimetype == nil) then
        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(500)
        print("no file infos in data base found")
        return
    end

    local newFileName, errMsg = generateFileName(data)

    if (errMsg ~= nil) then
        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(errMsg)
        print("filename on serverside is invalid")
        return
    end

    local fileContent, errMsg = readFile(serverFilename)

    if (errMsg ~= nil) then
        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(errMsg)
        print("file or path does not exist")
        return
    end

    if (fileContent == nil) then
        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(500)
        print("failed to read file")
        return
    end

    server.setBuffer()
    server.sendHeader('Access-Control-Expose-Headers','Content-Disposition');
    server.sendHeader('Content-type', mimetype)
    server.sendHeader('Content-Disposition', "attachment; filename=" .. newFileName)
    server.sendStatus(200)
    server.print(fileContent)
    return
end

function getMetaDataFileResource()
    local id = string.match(uri, "%d+")
    local data = readRes(id)

    -- Data does not exist in the database
    if (data == nil) then
        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(404)
        print("no resource found in database")
        return
    end

    local newFileName, errMsg = generateFileName(data)

    if (errMsg ~= nil) then
        newFileName = "metadata.xml"
    else
        local startVal, endVal = string.find(newFileName, "%.")
        if (startVal ~= nil) and (endVal ~= nil) then
            newFileName = string.sub(newFileName, 0, endVal) .. "xml"
        end
    end

    local fileContent = getXMLContent(data)

    server.setBuffer()
    server.sendHeader('Access-Control-Expose-Headers','Content-Disposition');
    server.sendHeader('Content-type', 'text/xml')
    server.sendHeader('Content-Disposition', "attachment; filename=" .. newFileName)
    server.sendStatus(200)
    server.print(fileContent)
    return
end

-- Checking of the url and appling the appropriate function
baseURL = "^/api"
uri = server.uri

routes = {}
routes[baseURL .. "/resources$"] = getResources
routes[baseURL .. "/resources/%d+$"] = getResource
routes[baseURL .. "/resources/%d+/file$"] = getFileResource
routes[baseURL .. "/resources/%d+/metadata"] = getMetaDataFileResource

for route, func in pairs(routes) do
    if (string.match(uri, route) ~= nil) then
        func()
        return
    end
end

-- Fails if no url matches
server.sendStatus(404)
