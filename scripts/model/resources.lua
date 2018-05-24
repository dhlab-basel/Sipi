require "./model/query"

dbPath = "testDB/testData.db"
tableName = "resource"

-------------------------------------------------------------------------------
--|                           CRUD Operations                               |--
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Creates a new resource in the database
-- @param   'parameters' (table):  table with name of parameter and value
-- @return  (string): ID of the created resource
-------------------------------------------------------------------------------
function createRes(parameters)
    local db = sqlite(dbPath, "RW")
    local qry = db << insertQuery(parameters, tableName)
    local row = qry()

    qry = db << lastInsertedQuery()
    row = qry()
    --    print(table.concat(row, ", "))

    qry =~ qry;
    db =~ db;

    return row[0]
end

-------------------------------------------------------------------------------
-- Reads a resource from the database
-- @param   'id' (table): ID of the resource
-- @return  'data' (table): returns the data with dublin core fields
-------------------------------------------------------------------------------
function readRes(id)
    local db = sqlite(dbPath, "RW")
    local qry = db << selectIDQuery(id, tableName)
    local row = qry()
    local data

    if (row ~= nil) then
        -- ACHTUNG: "row[0]" IST EIGENTLICH LUA-SYNTAKTISCH FALSCH
        data = {}
        data["id"] = row[0]
        data["title"] = row[1]
        data["creator"] = row[2]
        data["subject"] = row[3]
        data["description"] = row[4]
        data["publisher"] = row[5]
        data["constributor"] = row[6]
        data["date"] = row[7]
        data["type"] = row[8]
        data["format"] = row[9]
        data["identifier"] = row[10]
        data["source"] = row[11]
        data["language"] = row[12]
        data["relation"] = row[13]
        data["coverage"] = row[14]
        data["rights"] = row[15]
        data["filename"] = row[16]
        data["mimetype"] = row[17]
    end

    -- delete query and free prepared statment
    qry =~ qry;
    -- delete the database connection
    db =~ db;

    return data
end

-------------------------------------------------------------------------------
-- Reads all the resources from the database
-- @param   'parameters' (table): table with name of parameter and value
-- @return  'data' (table): returns all the data with dublin core fields
-------------------------------------------------------------------------------
function readAllRes(parameters)
    local db = sqlite(dbPath, "RW")
    local qry

    if (#parameters ~= 0) then
        qry = db << selectConditionQuery(andOperator(parameters), tableName)
    else
        qry = db << selectAllQuery(tableName)
    end

    local row = qry()
    local allData = {}

    while (row) do
        local data = {}
        data["id"] = row[0]
        data["title"] = row[1]
        data["creator"] = row[2]
        data["subject"] = row[3]
        data["description"] = row[4]
        data["publisher"] = row[5]
        data["constributor"] = row[6]
        data["date"] = row[7]
        data["type"] = row[8]
        data["format"] = row[9]
        data["identifier"] = row[10]
        data["source"] = row[11]
        data["language"] = row[12]
        data["relation"] = row[13]
        data["coverage"] = row[14]
        data["rights"] = row[15]
        table.insert(allData, data)
        row = qry()
    end

    qry =~ qry;
    db =~ db;

    return allData
end

-------------------------------------------------------------------------------
-- Updates a resource from the database
-- @param   'id' (table): ID of the resource
-- @param   'parameters' (table): table with name of parameter and value
-------------------------------------------------------------------------------
function updateRes(id, parameters)
    local db = sqlite(dbPath, "RW")
    local qry = db << updateQuery(id, parameters, tableName)
    local row = qry()

    qry =~ qry;
    db =~ db;
end

-------------------------------------------------------------------------------
-- Deletes a resource from the database
-- @param   'id' (table): ID of the resource
-------------------------------------------------------------------------------
function deleteRes(id)
    local db = sqlite(dbPath, "RW")
    local qry = db << deleteQuery(id, tableName)
    local row = qry()

    qry =~ qry;
    db =~ db;
end
