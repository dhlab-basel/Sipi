require "./model/query"

dbPath = "testDB/testData.db"
tableName = "collection"

-------------------------------------------------------------------------------
--|                           CRUD Operations                               |--
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Creates a new collection in the database
-- @param   'parameters' (table):  table with name of parameter and value
-- @return  (string): ID of the created collection
-------------------------------------------------------------------------------
function createCol(parameters)
    parameters["isLeaf"] = 1
    local db = sqlite(dbPath, "RW")
    local qry = db << insertQuery(parameters, "collection")
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
    local qry = db << selectIDQuery(id, "collection")
    local row = qry()
    local data

    if (row ~= nil) then
        -- ACHTUNG: "row[0]" IST EIGENTLICH LUA-SYNTAKTISCH FALSCH
        data = {}
        data["id"] = row[0]
        data["name"] = row[1]
        data["collection_id"] = row[2]
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
function readAllCol(parameters)
    local db = sqlite(dbPath, "RW")
    local firstCond = "id!=0"

    for key, param in pairs(parameters) do

        for k, p in pairs(param) do
            print(k,p)
        end

        local query
        if (param[3] == "EQ") then
            query = equal(param[2], param[4])
        elseif (param[3] == "!EQ") then
            query = notEqual(param[2], param[4])
        elseif (param[3] == "LIKE") then
            query = like(param[2], param[4])
        elseif (param[3] == "!LIKE") then
            query = notLike(param[2], param[4])
        elseif (param[3] == "EX") then
            query = exists(param[2], param[4])
        elseif (param[3] == "!EX") then
            query = notExists(param[2], param[4])
        end

        if (param[1] == "AND") then
            firstCond = andOperator({firstCond, query})
        elseif (param[1] == "OR") then
            firstCond = orOperator({firstCond, query})
        else
            print("fail")
        end

    end

    print(selectConditionQuery(firstCond, "collection"))
    local qry = db << selectConditionQuery(firstCond, "collection")
    local row = qry()
    local allData = {}

    while (row) do
        local data = {}
        data["id"] = row[0]
        data["name"] = row[1]
        data["collection_id"] = row[2]
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
    local qry = db << updateQuery(id, parameters, "collection")
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
    local qry = db << deleteQuery(id, "collection")
    local row = qry()

    qry =~ qry;
    db =~ db;
end