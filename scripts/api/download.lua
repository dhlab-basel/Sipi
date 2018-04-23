print("-------DOWNLOAD script------")

local success, result = server.http("GET", "http://localhost:1024/test/lena512.tif/pct:40,20,60,60/full/0/default.jpg");
--local success, result = server.http("GET", "https://www.zooroyal.at/magazin/wp-content/uploads/2017/06/hund-im-sommer-760x560.jpg");

print(table.concat(result, ", "))

server.setBuffer()
server.sendHeader('Content-type', 'image/jpeg')
server.sendStatus(200)
server.print(result.body)
