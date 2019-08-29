server.setBuffer()
server.log('token.lua called!', server.loglevel.debug)

if server.cookies then
    cookie_raw = server.cookies['sipi-auth']
    success, cookie_val = server.decode_jwt(cookie_raw)
    if not success then
        server.sendStatus(500)
        server.log(tokenVal, server.loglevel.err)
        return false
    end
end

tokenData = {
    type = 'token',
    allow = 'true',
    user = cookie_val['user']
}
success, tokenVal = server.generate_jwt(tokenData)
if not success then
    server.sendStatus(500)
    server.log(tokenVal, server.loglevel.err)
    return false
end

if server.get['messageId'] and server.get['messageId'] then
    messageId = server.get['messageId']
    origin = server.get['origin']

    server.sendHeader('Content-type', 'text/html')
    server.sendStatus(200)
    server.print('<html><body><script>')
    server.print('window.parent.postMessage({"messageId": '
            .. messageId .. ', "accessToken": "' .. tokenVal .. '", "expiresIn": 3600}, "' .. origin .. '");')
    server.print('</script></body></html>')
else
    result_table = {
        accessToken = tokenVal,
        expiresIn = 3600
    }
    success, result = server.table_to_json(result_table)
    if not success then
        server.sendStatus(500)
        server.log(result, server.loglevel.err)
        return false
    end
    server.sendHeader('Content-type', 'application/json')
    server.sendStatus(200)
    server.print(result)
end

return true;
