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
        data["collection_id"] = row[16]
        data["filename"] = row[17]
        data["mimetype"] = row[18]
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

    local qry = db << selectConditionQuery(trivialCond, tableName)
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
        data["collection_id"] = row[16]
        data["filename"] = row[17]
        data["mimetype"] = row[18]
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
