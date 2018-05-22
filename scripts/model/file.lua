
function uploadFile(parameters)
    for fileIndex, fileParam in pairs(server.uploads) do

        if (fileParam["filesize"] > 600000) then
            local table = {}
            table["data"] = { }
            table["status"] = "file too big"

            local success, jsonstr = server.table_to_json(table)
            if not success then
                server.sendStatus(500)
                server.log(jsonstr, server.loglevel.err)
                return false
            end

            server.sendHeader('Content-type', 'application/json')
            server.sendStatus(413)
            server.print(jsonstr)
            return
        end

        print(fileParam["origname"])
        -- Copy file (only on unix systems)
        --    os.execute("cp vogel.jpg kopie.jpg");

        local startPos, endPos = string.find(fileParam["origname"], "%.")
        local fileEnding = string.sub(fileParam["origname"], endPos+1, string.len(fileParam["origname"]))

        local tmpdir = 'data/tmp/'
        local success, exists = server.fs.exists(tmpdir)

        if not success then
            -- Was ist das für ein Fall?
        end

        if not exists then
            local success, errmsg = server.fs.mkdir(tmpdir, 511)
            if not success then
                --            server.print("<br> Directory couldn't be created <br>")
            end
        else
            --        server.print("directory exists <br>")
        end

        local success, uuid62 = server.uuid62()
        if not success then
        end

        print(uuid62 .. '_' .. string.gsub(parameters['title'], " ", "-"))

        parameters["filename"] = uuid62 .. '.' .. fileEnding
        parameters["mimetype"] = fileParam["mimetype"]
        parameters["filesize"] = fileParam["filesize"]

        local tmppath =  tmpdir .. parameters["filename"]

        local success, errmsg = server.copyTmpfile(fileIndex, tmppath)
        if not success then
        else
        end

        return parameters

    end

end
