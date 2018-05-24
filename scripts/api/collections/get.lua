print("---- GET collections script ----")

require "./model/collections"



local table = {}
table["data"] =  readAllCol()

server.setBuffer()

local success, jsonstr = server.table_to_json(table)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

server.sendHeader('Content-type', 'application/json')
server.sendStatus(200)
server.print(jsonstr)