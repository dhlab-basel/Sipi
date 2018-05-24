require "./model/query"

dbPath = "testDB/testData.db"
tableName = "collections"

-------------------------------------------------------------------------------
--|                           CRUD Operations                               |--
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Creates a new collection in the database
-- @param   'parameters' (table):  table with name of parameter and value
-- @return  (string): ID of the created collection
-------------------------------------------------------------------------------
function createCol(parameters)
    local db = sqlite(dbPath, "RW")
    local qry = db << insertQuery(parameters, tableName)
    local row = qry()

    qry = db << lastInsertedQuery()
    row = qry()

    qry =~ qry;
    db =~ db;

    return row[0]
end

-------------------------------------------------------------------------------
-- Reads a collection from the database
-- @param   'id' (table): ID of the collection
-- @return  'data' (table): returns the data of the collection
-------------------------------------------------------------------------------
function readCol(id)
    local db = sqlite(dbPath, "RW")
    local qry = db << selectIDQuery(id, tableName)
    local row = qry()
    local data

    if (row ~= nil) then
        -- ACHTUNG: "row[0]" IST EIGENTLICH LUA-SYNTAKTISCH FALSCH
        data = {}
        data["id"] = row[0]
        data["name"] = row[1]
        data["id_collections"] = row[2]
        data["isLeaf"] = row[3]
    end

    -- delete query and free prepared statment
    qry =~ qry;
    -- delete the database connection
    db =~ db;

    return data
end

-------------------------------------------------------------------------------
-- Reads all the collections from the database
-- @param   'parameters' (table): table with name of parameter and value
-- @return  'data' (table): returns all the collections
-------------------------------------------------------------------------------
function readAllCol()
    local db = sqlite(dbPath, "RW")
    local qry = db << selectAllQuery(tableName)
    local row = qry()
    local allData = {}

    while (row) do
        local data = {}
        data["id"] = row[0]
        data["name"] = row[1]
        data["id_collections"] = row[2]
        data["isLeaf"] = row[3]
        table.insert(allData, data)
        row = qry()
    end

    qry =~ qry;
    db =~ db;

    return allData
end

-------------------------------------------------------------------------------
-- Updates a collection from the database
-- @param   'id' (table): ID of the collection
-- @param   'parameters' (table): table with name of parameter and value
-------------------------------------------------------------------------------
function updateCol(id, parameters)
    local db = sqlite(dbPath, "RW")
    local qry = db << updateQuery(id, parameters, tableName)
    local row = qry()

    qry =~ qry;
    db =~ db;
end

-------------------------------------------------------------------------------
-- Deletes a collection from the database
-- @param   'id' (table): ID of the collection
-------------------------------------------------------------------------------
function deleteCol(id)
    local db = sqlite(dbPath, "RW")
    local qry = db << deleteQuery(id, tableName)
    local row = qry()

    qry =~ qry;
    db =~ db;
end