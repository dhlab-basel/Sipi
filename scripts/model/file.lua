
path = "./data/tmp/"

-------------------------------------------------------------------------------
--|                           CRUD Operations                               |--
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Creates a file on the file system
-- @param   'parameters' (table):  table with name of parameter and value
-- @return  'parameters' (table):  table with added metadata about the file
-------------------------------------------------------------------------------
function createFile(parameters)
    for fileIndex, fileParam in pairs(server.uploads) do

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

-------------------------------------------------------------------------------
-- Reads the content of the file
-- @param   'filename' (string):  name of the file with the file ending
-- @return  'content': content of the file
-------------------------------------------------------------------------------
function readFile(filename)
    local file = io.open(path .. filename)
    io.input(file)
    local content = io.read("*a")
    io.close(file)

    return content
end

-------------------------------------------------------------------------------
-- Updates the file by replacing the old file with the new one from server.uploads
-- @param   'parameters' (table):  table with name of parameter and value
-- @param   'filename' (string):  name of the file with the file ending
-- @return  'parameters' (table):  table with updated data and metadata about the file
-------------------------------------------------------------------------------
function updateFile(parameters, filename)
    parameters = createFile(parameters)
    deleteFile(filename)
    return parameters
end

-------------------------------------------------------------------------------
-- Deletes the file on the file system
-- @param   'filename' (string):  name of the file with the file ending
-------------------------------------------------------------------------------
function deleteFile(fileName)
    if (os.remove(path .. fileName)) then
        print("file deleted")
    else
        print("file could not be deleted")
    end
end
