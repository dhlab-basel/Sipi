print("-------POST script------")

function createID()
    return math.random(10, 1000)
end

function getQuery(title, date)
    return 'INSERT INTO pdfObject (title, date) values (("'.. title ..'"), ("'.. date ..'"));'
end

function checkInsertQuery(id)
    return 'SELECT * FROM pdfObject WHERE id = "'.. id .. '"'
end

parameters = {}
local newID

for key,value in pairs(server.post) do
print(key, value)
    if key == '"title"' then
        parameters[1] = value
    elseif key == '"date"' then
        parameters[2] = value
    else
        print("Something is wrong: " .. value .. key)
    end

end

--print(table.concat(parameters, ", "))
if server.uploads ~= nil then

    local db = sqlite("testDB/testData.db", "RW")
    local qry = db << getQuery(parameters[1], parameters[2])
    local row = qry()

    qry = db << 'SELECT last_insert_rowid()'
    row = qry()

    newID = row[0]

--    qry = db << checkInsertQuery(createID())
--    row = qry()
--
--    print(table.concat(row, ", "))


    qry = ~qry -- delete query and free prepared statment
    db = ~db -- delete the database connection



    for pdfindex, pdfparam in pairs(server.uploads) do
        print(pdfparam["filesize"])
        if (pdfparam["filesize"] < 100000) then
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

        -- PDF is too big
        else
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

    end
else

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

table = {}

element1 = {}
element1["id"] = newID
element1["title"] = parameters[1]
element1["date"] = parameters[2]

table["data"] = element1
table["status"] = "successful"



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
