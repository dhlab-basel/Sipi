require "./model/query"

dbPath = "testDB/testData.db"
resTable = "resource"

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
    local qry = db << insertQuery(parameters, resTable)
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
    local qry = db << selectIDQuery(id, resTable)
    local row = qry()
    local data

    if (row ~= nil) then
        data = getDataFormat(row)
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
        local paramName = param[1]
        local compOp = param[2]
        local value1 = param[3]
        local value2 = param[4]

        if (param[1] == "date") then
            if (compOp == "EQ") then
                local s1 = equal(paramName, value1)
                local s2 = equal(paramName, value1)
                statement = "(" .. andOperator({s1, s2}) .. ")"
            elseif (compOp == "LIKE") then
                -- For exact year
                local s1 = equal("date_start", value1)
                local s2 = equal("date_end", value1)
                local statement1 = "(" .. andOperator({s1, s2}) .. ")"

                -- For period
                local s3 = greaterThanEqual(value1, "date_start")
                local s4 = lessThanEqual(value1, "date_end")
                local statement2 = "(" .. andOperator({s3, s4}) .. ")"

                statement = orOperator({statement1, statement2})
            elseif (compOp == "NULL") then
                local s1 = isNull("date_start")
                local s2 = isNull("date_end")
                statement = "(" .. andOperator({s1, s2}) .. ")"
            elseif (compOp == "!NULL") then
                local s1 = isNotNull("date_start")
                local s2 = isNotNull("date_end")
                statement = "(" .. andOperator({s1, s2}) .. ")"
            elseif (compOp == "GT") then
                local s1 = greaterThan("date_start", value1)
                local s2 = greaterThan("date_end", value1)
                statement = "(" .. orOperator({s1, s2}) .. ")"
            elseif (compOp == "GT_EQ") then
                local s1 = greaterThanEqual("date_start", value1)
                local s2 = greaterThanEqual("date_end", value1)
                statement = "(" .. orOperator({s1, s2}) .. ")"
            elseif (compOp == "LT") then
                local s1 = lessThan("date_start", value1)
                local s2 = lessThan("date_end", value1)
                statement = "(" .. orOperator({s1, s2}) .. ")"
            elseif (compOp == "LT_EQ") then
                local s1 = lessThanEqual("date_start", value1)
                local s2 = lessThanEqual("date_end", value1)
                statement = "(" .. orOperator({s1, s2}) .. ")"
            elseif (compOp == "BETWEEN") then
                -- For exact year and period, that have only one overlap
                local s1 = betweenDates("date_start", value1, value2)
                local s2 = betweenDates("date_end", value1, value2)

                local statement1 = "(" .. orOperator({s1, s2}) .. ")"

                -- For period that are within the between values
                local s3 = lessThanEqual("date_start", value1)
                local s4 = greaterThanEqual("date_end", value2)

                local statement2 = "(" .. andOperator({s3, s4}) .. ")"
                statement = orOperator({statement1, statement2})
            end

            -- Search in collection -> CROSS JOIN

        else
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
            end
        end

        trivialCond = andOperator({trivialCond, statement})
    end

    print(selectConditionQuery(trivialCond, resTable))

    local qry = db << selectConditionQuery(trivialCond, resTable)
    local row = qry()
    local allData = {}

    while (row) do
        table.insert(allData, getDataFormat(row))
        row = qry()
    end

    qry =~ qry;
    db =~ db;

    return allData
end

-------------------------------------------------------------------------------
-- Reads all the resources from the database with a full text search
-- @param   'searchword' (string): word which should be looked for in all the column
-- @return  'data' (table): returns all the data with dublin core fields
-------------------------------------------------------------------------------
function readAllResFullText(searchword)
    local db = sqlite(dbPath, "RW")
    local trivialCond = "id==0"
    local statement

    local parameters = {"title", "creator", "subject", "description", "publisher", "contributor", "type", "format", "identifier", "source", "language", "relation", "coverage", "rights", "filename", "mimetype"}

    if (searchword ~= nil) and (searchword ~= "") then
        for k, paramName in pairs(parameters) do
            statement = like(paramName, searchword)
            trivialCond = orOperator({trivialCond, statement})
        end

        -- Search statement in time period if searchword is number
        if (tonumber(searchword) ~= nil) then
            local s1 = greaterThanEqual(searchword, "date_start")
            local s2 = lessThanEqual(searchword, "date_end")
            statement = "(" .. andOperator({s1, s2}) .. ")"
            trivialCond = orOperator({trivialCond, statement})
        end

        -- Search in collection -> CROSS JOIN

    else
        trivialCond = "id!=0"
    end

    print(selectConditionQuery(trivialCond, resTable))

    local qry = db << selectConditionQuery(trivialCond, resTable)
    local row = qry()
    local allData = {}

    while (row) do
        table.insert(allData, getDataFormat(row))
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
    local qry = db << updateQuery(id, parameters, resTable)
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
    local qry = db << deleteQuery(id, resTable)
    local row = qry()

    qry =~ qry;
    db =~ db;
end

-------------------------------------------------------------------------------
--|                            Internal Function                            |--
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Gets the data from sqlite into a table
-- @param   'row' (table): row of the sqlite database of a resource
-- @return  'data' (table): returns the data with dublin core fields
-------------------------------------------------------------------------------
function getDataFormat(row)
    local data = {}
    -- ACHTUNG: "row[0]" IST EIGENTLICH LUA-SYNTAKTISCH FALSCH
    data["id"] = row[0]
    data["title"] = row[1]
    data["creator"] = row[2]
    data["subject"] = row[3]
    data["description"] = row[4]
    data["publisher"] = row[5]
    data["contributor"] = row[6]
    data["date_start"] = row[7]
    data["date_end"] = row[8]
    data["type"] = row[9]
    data["format"] = row[10]
    data["identifier"] = row[11]
    data["source"] = row[12]
    data["language"] = row[13]
    data["relation"] = row[14]
    data["coverage"] = row[15]
    data["rights"] = row[16]
    data["collection_id"] =  { ["id"] = row[17], ["url"] = "/api/collections/" .. row[17]}
    data["filename"] = row[18]
    data["mimetype"] = row[19]
    data["filesize"] = row[20]
    data["url"] = "/api/resources/".. row[0] .."/file"

    return data
end
