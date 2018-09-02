print("---- POST login script ----")

local name = "admin"
local pw = "1234"

local sendName, sendPW
for key, value in pairs(server.post) do
    if (key == "name") then
        sendName = value
    elseif (key == "password") then
        sendPW = value
    end
end

if (sendName == name) and (sendPW == pw) then
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(200)
else
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(401)
end
