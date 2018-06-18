print("---- OPTIONS collections script ----")

local table = {}
local element = {}

table["data"] = { element }
table["status"] = "successful"

local success, jsonstr = server.table_to_json(table)
if not success then
    server.sendStatus(500)
    server.log(jsonstr, server.loglevel.err)
    return false
end

server.setBuffer()
server.sendHeader('Content-type', 'application/json')
server.sendHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS')
server.sendHeader('Access-Control-Allow-Origin', '*')
server.sendStatus(200)
server.print(jsonstr)
