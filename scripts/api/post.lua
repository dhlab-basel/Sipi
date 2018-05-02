print("-------POST script------")

require "../model/database"

function createID()
    return math.random(10, 1000)
end

local newID

-- Get parameters
local element = {}
for key,value in pairs(server.post) do
    if (key == '"date"') then
        element["date"] = value
    elseif (key == '"title"') then
        element["title"] = value
    else
        print("fail")
    end
end

--print(table.concat(parameters, ", "))
if server.uploads == nil then
    local table = {}
    table["data"] = { }
    table["status"] = "no pdf attached"

    local success, jsonstr = server.table_to_json(table)
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

newID = createData(element)

for pdfindex, pdfparam in pairs(server.uploads) do

    if (pdfparam["filesize"] > 100000) then

        local table = {}
        table["data"] = { }
        table["status"] = "pdf too big"

        local success, jsonstr = server.table_to_json(table)
        if not success then
            server.sendStatus(500)
            server.log(jsonstr, server.loglevel.err)
            return false
        end

        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(413)
        server.print(jsonstr)
        return

    end

    print(pdfparam["origname"])
    --    server.print(config.imgroot .. "<br>")

    local tmpdir = 'pdf/tmp/'
    local success, exists = server.fs.exists(tmpdir)

    if not success then
        --        server.print("success = false")
    end
    if not exists then
        local success, errmsg = server.fs.mkdir(tmpdir, 511)
        if not success then
            --            server.print("<br> Directory couldn't be created <br>")
        end
    else
        --        server.print("directory exists <br>")
    end

    local success, uuid62 = server.uuid62()
    if not success then
        --        server.print("Not successful <br>")
    end

    local tmppath =  tmpdir .. uuid62 .. '.pdf'
    --    server.print(tmppath .. "<br>")

    local success, errmsg = server.copyTmpfile(pdfindex, tmppath)
    if not success then
        --        server.print("Not successful<br>")
    else
        --        server.print("successful<br>")
    end

end


table = {}
table["data"] = readData(newID)

if (table["data"] ~= nil) and (table["data"]["title"] == element["title"]) and (table["data"]["date"] == element["date"]) then
    table["status"] = "successful"
else
    table["status"] = "unsuccessful"
end


local success, jsonstr = server.table_to_json(table)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

--print(jsonstr)

server.sendHeader('Content-type', 'application/json')
server.sendStatus(201)
server.print(jsonstr)
