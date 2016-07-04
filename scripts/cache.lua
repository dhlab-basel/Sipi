--
-- Created by IntelliJ IDEA.
-- User: rosenth
-- Date: 01/07/16
-- Time: 23:00
-- To change this template use File | Settings | File Templates.
--

--[[
auth = server.requireAuth()
if (auth.status ~= 'BEARER') then
    server.sendStatus(401)
    server.sendHeader('WWW-Authenticate', 'Bearer')
    server.print("Wrong credentials!")
    return -1
end
jwt = auth.token
jwt = server.decode_jwt(intoken)
if (jwt.iss ~= 'sipi.unibas.ch') or (jwt.aud ~= 'knora.org') or (jwt.user ~= config.adminuser) then
    server.sendStatus(401)
    server.sendHeader('WWW-Authenticate', 'Bearer')
    return -1
end
]]--

if server.method == 'GET' then
    print("GET method!!!!!")
    if server.get and (server.get.sort == 'atasc') then
        flist = cache.filelist('AT_ASC')
    elseif server.get and (server.get.sort == 'atdesc') then
        flist = cache.filelist('AT_DESC')
    elseif server.get and (server.get.sort == 'fsasc') then
        flist = cache.filelist('FS_ASC')
    elseif server.get and (server.get.sort == 'fsdesc') then
        flist = cache.filelist('FS_DESC')
    else
        print("Before getting filelist")
        flist = cache.filelist('AT_ASC')
        print("After getting filelist")
        for index,value in pairs(flist) do
            print("index: ", index, " value: ")
            for u,v in pairs(value) do
                print("  ", u, "  ", v)
            end
        end
    end


    jsonstr = server.table_to_json(flist)

    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(200)
    server.print(jsonstr)
elseif server.method == 'DELETE' then
    if server.content and server.content_type == 'application/json' then
        todel = server.json_to_table(server.content)
        print('TODEL=', todel)
        for index,canonical in pairs(todel) do
        --    cache.delete(canonical)
            print('DELETING ', index, ' ', canonical)
        end
        result = {
            status = 'OK'
        }
        jsonresult = server.table_to_json(result)
        server.sendHeader('Content-type', 'application/json')
        server.sendStatus(200);
        server.print(jsonresult)

    end
end

