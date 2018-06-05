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
    local trivialCond = "id!=0"

    for key, param in pairs(parameters) do

        local statement
        if (param[3] == "EQ") then
            statement = equal(param[2], param[4])
        elseif (param[3] == "!EQ") then
            statement = notEqual(param[2], param[4])
        elseif (param[3] == "LIKE") then
            statement = like(param[2], param[4])
        elseif (param[3] == "!LIKE") then
            statement = notLike(param[2], param[4])
        elseif (param[3] == "NULL") then
            statement = isNull(param[2])
        elseif (param[3] == "!NULL") then
            statement = isNotNull(param[2])
        elseif (param[3] == "GT") then
            statement = greaterThan(param[2], param[4])
        elseif (param[3] == "GT_EQ") then
            statement = greaterThanEqual(param[2], param[4])
        elseif (param[3] == "LT") then
            statement = lessThan(param[2], param[4])
        elseif (param[3] == "LT_EQ") then
            statement = lessThanEqual(param[2], param[4])
        elseif (param[3] == "BETWEEN") then
            statement = betweenDates(param[4], param[5])
        end

        if (key == 1) then
            trivialCond = andOperator({trivialCond, statement})
        else
            if (param[1] == "AND") then
                trivialCond = andOperator({trivialCond, statement})
            elseif (param[1] == "OR") then
                trivialCond = orOperator({trivialCond, statement})
            end
        end

    end

    local qry = db << selectConditionQuery(trivialCond, "collection")
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