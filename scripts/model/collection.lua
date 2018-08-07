require "./model/query"

dbPath = "testDB/testData.db"
colTable = "collection"

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
    local qry = db << insertQuery(parameters, colTable)
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
    local qry = db << selectIDQuery(id, colTable)
    local row = qry()
    local data

    if (row ~= nil) then
        -- ACHTUNG: "row[0]" IST EIGENTLICH LUA-SYNTAKTISCH FALSCH
        data = {}
        data["id"] = row[0]
        data["name"] = row[1]
        data["collection_id"] =  { ["id"] = row[2], ["url"] = "/api/collections/" .. row[2]}
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
    local trivialCond = "id!=0"

    for key, param in pairs(parameters) do

        local statement
        local paramName = param[1]
        local compOp = param[2]
        local value1 = param[3]
        local value2 = param[4]

        if (compOp == "EQ") then
            statement = equal(paramName, value1)
        elseif (compOp == "!EQ") then
            statement = notEqual(paramName, value1)
        elseif (compOp == "LIKE") then
            statement = like(paramName, value1)
        elseif (compOp == "!LIKE") then
            statement = notLike(paramName, value1)
        elseif (compOp == "NULL") then
            statement = isNull(paramName)
        elseif (compOp == "!NULL") then
            statement = isNotNull(paramName)
        elseif (compOp == "GT") then
            statement = greaterThan(paramName, value1)
        elseif (compOp == "GT_EQ") then
            statement = greaterThanEqual(paramName, value1)
        elseif (compOp == "LT") then
            statement = lessThan(paramName, value1)
        elseif (compOp == "LT_EQ") then
            statement = lessThanEqual(paramName, value1)
        elseif (compOp == "BETWEEN") then
            statement = betweenDates(paramName, value1, value2)
        end

        trivialCond = andOperator({trivialCond, statement})
    end

    local qry = db << selectConditionQuery(trivialCond, colTable)
    local row = qry()
    local allData = {}

    while (row) do
        local data = {}
        data["id"] = row[0]
        data["name"] = row[1]
        data["collection_id"] =  { ["id"] = row[2], ["url"] = "/api/collections/" .. row[2]}
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
    local qry = db << updateQuery(id, parameters, colTable)
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
    local qry = db << deleteQuery(id, colTable)
    local row = qry()

    qry =~ qry;
    db =~ db;
end