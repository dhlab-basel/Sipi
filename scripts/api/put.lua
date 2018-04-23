print("-------PUT script------")

function doesExistsQuery(id)
    return 'SELECT * FROM pdfObject WHERE id= "'.. id .. '"'
end

function putQuery(id, title, date)
    return 'UPDATE pdfObject SET title="'.. title ..'", date="'.. date ..'" WHERE id= "'.. id .. '";'
end

for key,value in pairs(server.post) do
    print(key, value)
end

if server.uploads ~= nil then
    for pdfindex,pdfparam in pairs(server.uploads) do
        print(pdfparam["origname"])

        --    os.rename("bird.jpg", "vogel.jpg")
        --    res = os.remove("vogel 2.jpg")

        local db = sqlite("testDB/testData.db", "RW")
        local qry = db << putQuery(82, "Sherlock Holmes 34", "2018")
        local row = qry()
        print(row)

        qry = ~qry -- delete query and free prepared statment
        db = ~db -- delete the database connection
    end
else

end

table = {}

element1 = {}
element1["id"] = "1"
element1["title"] = "Phantom of the Opera"
element1["date"] = "04.12.2006"

table["subjects"] = { element1 }
table["status"] = "successful"

local success, jsonstr = server.table_to_json(table)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

--print(jsonstr)

server.sendHeader('Content-type', 'application/json')
server.sendStatus(200)
server.print(jsonstr)
