require "send_response"

success, errmsg = server.setBuffer()
if not success then
    server.log("server.setBuffer() failed: " .. errmsg, server.loglevel.LOG_ERR)
    return
end

inputPath = config.imgroot .. '/knora/Leaves.jpg'
outputPath = config.imgroot .. '/knora/_Leaves.jpx'

success, img = SipiImage.new(inputPath)
if not success then
    server.log("SipiImage.new() failed: " .. img, server.loglevel.LOG_ERR)
    return
end

success, dummy = img:write(outputPath)
if not success then
    server.log("SipiImage.write() failed: " .. dummy, server.loglevel.LOG_ERR)
    return
end

result = {
    status = 0,
    message = 'OK'
}

send_success(result)