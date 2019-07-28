
server.setBuffer()
cookieData = {
    type = 'cookie',
    allow = 'true',
    user = server.post['username']
}
success, cookieVal = server.generate_jwt(cookieData)
if not success then
    server.sendStatus(500)
    server.log(cookieVal, server.loglevel.err)
    return false
end

server.sendHeader('Content-type', 'text/html')
server.sendStatus(200);
server.sendCookie('sipi-auth', cookieVal, { domain = 'localhost', path = '/' })
server.print('<html>')
server.print('<head><title>do-cookie</title></head>')
server.print('<script type="application/javascript">function closit() { window.close(); }</script>')
server.print('<body onload="closit()">done</body>')
server.print('</html>')

return true
