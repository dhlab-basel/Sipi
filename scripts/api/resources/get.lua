print("---- GET resources script ----")

-- Required external script files
require "./model/resource"
require "./model/parameter"
require "./model/file"

function isXmlAccepted()
    -- Checks if only xml is accepted in header
    for key, value in pairs(server.header) do
        if (key == "accept") then

            -- MimeType pattern for xml
            local xmlPattern1 = "^text/xml$"
            local xmlPattern2 = "^application/xml$"

            -- Searching for pattern in value of accept
            local i, j = string.find(value, xmlPattern1)
            local k, l = string.find(value, xmlPattern2)

            return (((i~=nil) and (j ~= nil)) or ((k~=nil) and (l~=nil)))
        end
    end
end

function sendDublinCoreXML()
    server.setBuffer()
    server.sendHeader("Content-Type", "text/xml")
    server.print("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")
    server.print("<resource>")
    server.print("<name>Zebra</name>")
    server.print("</resource>")
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

    -- returns XML response if it accepted in header
    if (isXmlAccepted()) then
        sendDublinCoreXML(table1["data"])
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

-- Checking of the url and appling the appropriate function
baseURL = "^/api"
uri = server.uri

routes = {}
routes[baseURL .. "/resources$"] = getResources
routes[baseURL .. "/resources/%d+$"] = getResource
routes[baseURL .. "/resources/%d+/file$"] = getFileResource

for route, func in pairs(routes) do
    if (string.match(uri, route) ~= nil) then
        func()
        return
    end
end

-- Fails if no url matches
server.sendStatus(404)
