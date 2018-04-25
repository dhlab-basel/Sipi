function fact(n)
    if n == 0 then
        return 1
    else
        return n * fact(n-1)
    end
end

function boll()
    if 2 == 2 then
        return 4 == 4
    end
end

--[[
print("Enter a number")
a = io.read("*n")
print(fact(a))
--]]

table = [[
    <table>
        <tr>
            <th>Monate</th>
            <th>Kosten</th>
        </tr>
        <tr>
            <td>Januar</td>
            <td>300$</td>
        </tr>
        <tr>
            <td>Februar</td>
            <td>100$</td>
        </tr>
    </table>
]]

page = [[
    <html>
        <head>
            <title>Der Titel</title>
        </head>
        <body>
            <table>
                <tr>
                    <th>Monate</th>
                    <th>Kosten</th>
                </tr>
                <tr>
                    <td>Januar</td>
                    <td>300$</td>
                </tr>
                <tr>
                    <td>Februar</td>
                    <td>100$</td>
                </tr>
            </table>
        </body>
    </html>
]]

--a = {}
--for i = 1, 1000
--    do a[i] = i*2
----    print(a[i])
--end
--
--s = "api/resources/"
--
--i, j = string.find(s, "api/resources/")
--local id
--if (i ~= nil) and (j ~= nil) then
--    i = tonumber(string.sub(s, j+1, string.len(s)))
--    if (i ~= nil) and (type(i)) == "number" then
--        id = i
--    end
--end

regexTable = {}
regexTable[1] = "api/resources"
regexTable[2] = "api/resources/"
regexTable[3] = "api/resources/0"
regexTable[4] = "api/resources/1"
regexTable[5] = "api/resources/00"
regexTable[6] = "api/resources/2"
regexTable[7] = "api/resources/183"
regexTable[8] = "api/resources/0.3"
regexTable[9] = "api/resources/1.5"
regexTable[10] = "api/resources//"
regexTable[11] = "api/resources/a"
regexTable[12] = "api/resources/a3"
regexTable[13] = "api/resources/67/"
regexTable[14] = "api/resources/13/file"
regexTable[15] = "api/resources/13/file/"
regexTable[16] = "api/resources/13/file"
regexTable[17] = "api/resources/1.3/file"
regexTable[18] = "api/resources/13/file/34"
regexTable[19] = "api/resources/0/file"
regexTable[20] = "api/resources/-0/file"
regexTable[21] = "api/resources/012345630/file"
regexTable[22] = "api/resources/2.0030/file"
regexTable[23] = "api/resources/2,345/file"
regexTable[24] = "api/resources/-0.3/file"
regexTable[25] = "api/resources/-0,3/file"

local filePattern = "api/resources/%d+/file$"
local idPattern = "api/resources/%d+$"
local resourcePattern = "api/resources$"

function hasFilePattern(url, filePattern)
    local i, j = string.find(url, filePattern)
    if (i~=nil) and (j ~= nil) then
        return string.match(url,"%d+")
    else
        return nil
    end
end

function hasIDPattern(url, idPattern)
    local z = string.match(url, idPattern)
    if (z~=nil) then
        return string.match(url, "%d+")
    else
        return nil
    end
end

function hasResourcePattern(url, pattern)
    local z = string.match(url, pattern)
    if z~=nil then
        return true
    else
        return false
    end
end


local id
for k,v in pairs(regexTable) do
    local id = hasFilePattern(v, filePattern)
    if (id ~= nil) then
        print(v .. "=> has FilePattern with = " .. id)
    else
        id = hasIDPattern(v, idPattern)
        if (id ~= nil) then
            print(v .. "=> has IDPattern with = " .. id)
        else
            if (hasResourcePattern(v, resourcePattern)) then
                print(v .. "=> has ressourcePattern")
            else
                print(v .. "=> FAIL")
            end
        end
    end
end


Monster = {}
Monster.__index = Monster
function Monster:Create()
    local this = {
        name = "orc",
        health = 10,
        attack = 3
    }
    setmetatable(this, Monster)
    return this
end

function Monster:WarCry()
    print(self.name .. ": Graahh!!!!")
end

--monster_01 = Monster:Create()
--monster_01: WarCry()
--
--print(monster_01)

local steuersatz = {
    zehn = 1.2,
    zwanzig = 1.4,
    vierzig = 1.8,
    sechszig = 2.2
}

steuersatz.steuerberechnung =  function(satz, geldmenge)
    if geldmenge < 10 then
        return 1.0 * geldmenge
    elseif 10 <= geldmenge and geldmenge < 20 then
        return satz.zehn * geldmenge
    elseif 20 <= geldmenge and geldmenge < 40 then
        return satz.zwanzig * geldmenge
    elseif 40 <= geldmenge and geldmenge < 60 then
        return satz.vierzig * geldmenge
    else
        return satz.sechszig * geldmenge
    end
end

--print(steuersatz:steuerberechnung(10))

meta =
{
    __index = function(table, key)
        return key * 2
    end
}

doubleTalbe = {}
setmetatable(doubleTalbe, meta)
--print(doubleTalbe[2])
--print(doubleTalbe[16])

parent = { name = "default name", title = "Mr" }
meta = { __index = parent }
child = { name = "Bob" }

setmetatable(child, meta)

--print(child.title, child.name)
child.name = nil
--print(child.title, child.name)


-- File reading
--io.input("vogel.jpg")
--s = io.read("*a")

--t = io.open("vogel.jpg", "r")
--print(t)

server.setBuffer()

server.sendHeader("Content-Type", "text/html")
server.print("<html>")
server.print("<head><title>St. Gallen</title></head>")
server.print("<body><h1>Stiftsbibliothekt - St. Gallen</h1>")
-- server.print("<table>")
-- server.print("<tr><th>Field</th><th>Value</th></tr>")
-- for k,v in pairs(server.header) do
--    server.print("<tr><td>", k, "</td><td>", v, "</td></tr>")
-- end
-- server.print("</table>")
server.print(table)
server.print("<hr/>")
server.print("URI: ", server.uri)
server.print("</body>")
server.print("</html>")
