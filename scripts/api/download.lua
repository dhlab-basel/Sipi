print("-------DOWNLOAD script------")

--local success, result = server.http("GET", "http://localhost:1024/test/lena512.tif/pct:40,20,60,60/full/0/default.jpg");

local success, result = server.http("GET", "https://hongkongdogrescue.com/wp-content/uploads/2016/04/Helena-300x300.jpg");

for key,value in pairs(result.header) do
    print(key, value)
end

-- File reading
io.input("vogel.jpg")
s = io.read("*a")

server.setBuffer()
--server.sendHeader('Content-type', 'application/pdf')
server.sendHeader('Content-type', 'image/jpeg')
server.sendStatus(200)

server.print(result.body)
--server.print(s)
