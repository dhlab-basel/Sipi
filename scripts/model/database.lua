--
-- Created by IntelliJ IDEA.
-- User: vijeinath
-- Date: 02.05.18
-- Time: 12:14
-- To change this template use File | Settings | File Templates.
--

dbPath = "testDB/testData.db"
tableName = "resource"

-------------------------------------------------------------------------------
--|                         Operator Builders                               |--
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Builds the EQUAL statement
-- @param   'name' (string):  name of the parameter
-- @param   'value' (string):  value of the parameter
-- @return  (string): EQUAL statement involving the parameter
-------------------------------------------------------------------------------
function equal(name, value)
    return name .. "='" .. value .. "'"
end

-------------------------------------------------------------------------------
-- Builds the not EQUAL statement
-- @param   'name' (string):  name of the parameter
-- @param   'value' (string):  value of the parameter
-- @return  (string): Not EQUAL statement involving the parameter
-------------------------------------------------------------------------------
function notEqual(name, value)
    return name .. "!='" .. value .. "'"
end

-------------------------------------------------------------------------------
-- Builds the LIKE statement
-- @param   'name' (string):  name of the parameter
-- @param   'value' (string):  value of the parameter
-- @return  (string): LIKE statement involving the parameter
-------------------------------------------------------------------------------
function like(name, value)
    return name .. ' LIKE "%' .. value .. '%"'
end

-------------------------------------------------------------------------------
-- Builds the not LIKE statement
-- @param   'name' (string):  name of the parameter
-- @param   'value' (string):  value of the parameter
-- @return  (string): not LIKE statement involving the parameter
-------------------------------------------------------------------------------
function notLike(name, value)
    return name .. ' NOT LIKE "%' .. value .. '%"'
end

-------------------------------------------------------------------------------
-- Builds the IS NULL statement
-- @param   'name' (string):  name of the parameter
-- @return  (string): IS NULL statement involving the parameter
-------------------------------------------------------------------------------
function isNull(name)
    return name .. ' IS NULL'
end

-------------------------------------------------------------------------------
-- Builds the IS NOT NULL statement
-- @param   'name' (string):  name of the parameter
-- @return  (string): IS NOT NULL statement involving the parameter
-------------------------------------------------------------------------------
function notNull(name)
    return name .. ' IS NOT NULL '
end

-------------------------------------------------------------------------------
-- Builds the GREATER THAN statement
-- @param   'name' (string):  name of the parameter
-- @param   'value' (number):  value of the parameter
-- @return  (string): GREATER THAN statement involving the parameter
-------------------------------------------------------------------------------
function greaterThan(name, value)
    return name .. ' > "' .. value .. '"'
end

-------------------------------------------------------------------------------
-- Builds the GREATER THAN EQUAL statement
-- @param   'name' (string):  name of the parameter
-- @param   'value' (number):  value of the parameter
-- @return  (string): GREATER THAN EQUAL statement involving the parameter
-------------------------------------------------------------------------------
function greaterThanEqual(name, value)
    return name .. ' >= "' .. value .. '"'
end

-------------------------------------------------------------------------------
-- Builds the LESS THAN statement
-- @param   'name' (string):  name of the parameter
-- @param   'value' (number):  value of the parameter
-- @return  (string): LESS THAN statement involving the parameter
-------------------------------------------------------------------------------
function lessThan(name, value)
    return name .. ' < "' .. value .. '"'
end

-------------------------------------------------------------------------------
-- Builds the LESS THAN EQUAL statement
-- @param   'name' (string):  name of the parameter
-- @param   'value' (number):  value of the parameter
-- @return  (string): LESS THAN EQUAL statement involving the parameter
-------------------------------------------------------------------------------
function lessThanEqual(name, value)
    return name .. ' <= "' .. value .. '"'
end

-------------------------------------------------------------------------------
-- Builds the BETWEEN statement
-- @param   'date1' (string):  start date
-- @param   'date2' (string):  end date
-- @return  (string): BETWEEN the dates statement
-------------------------------------------------------------------------------
function betweenDates(date1, date2)
    return 'BETWEEN "' .. date1 .. '" AND "' .. date2 .. '"'
end

-------------------------------------------------------------------------------
-- Concatinates all the parameter with the AND operator
-- @param   'parameters' (table):  table with parameter values
-- @return  (string): concatinated string involving all the parameters
-------------------------------------------------------------------------------
function andOperator(parameters)
    return table.concat(parameters, " AND ")
end

-------------------------------------------------------------------------------
-- Concatinates all the parameter with the OR operator
-- @param   'parameters' (table):  table with parameter values
-- @return  (string): concatinated string involving all the parameters
-------------------------------------------------------------------------------
function orOperator(parameters)
    return table.concat(parameters, " OR ")
end


-------------------------------------------------------------------------------
--|                            Query Builders                               |--
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Builds the SELECT all Query
-- @return  (string): SELECT query
-------------------------------------------------------------------------------
function selectAllQuery()
    return 'SELECT * FROM ' .. tableName
end

-------------------------------------------------------------------------------
-- Builds the SELECT id Query
-- @param   'id' (table):  id of the data
-- @return  (string): SELECT query with id condition
-------------------------------------------------------------------------------
function selectIDQuery(id)
    return 'SELECT * FROM ' .. tableName .. ' WHERE ' .. equal("id", id)
end

-------------------------------------------------------------------------------
-- Builds the SELECT Query
-- @param   'conditions' (string):  condition which can include serveral statements
-- @return  (string): SELECT query with concatinated conditions
-------------------------------------------------------------------------------
function selectConditionQuery(conditions)
    return 'SELECT * FROM ' .. tableName .. ' WHERE ' .. conditions
end

-------------------------------------------------------------------------------
-- Builds the INSERT Query
-- @param   'parameters' (table):  table with name of parameter and value
-- @return  (string): INSERT query with all parameters
-------------------------------------------------------------------------------
function insertQuery(parameters)
    local values = {}
    local names = {}
    for key, value in pairs(parameters) do
        if (value == "") then
            table.insert(values, '(null)')
        else
            table.insert(values, '("' .. value ..'")')
        end
        table.insert(names, key)
    end

    return 'INSERT INTO ' .. tableName .. ' (' .. table.concat(names, ", ") .. ') values (' .. table.concat(values, ",") .. ');'
end

-------------------------------------------------------------------------------
-- Builds the last inserted Query
-- @return  (string): query to get the last inserted row id
-------------------------------------------------------------------------------
function lastInsertedQuery()
    return 'SELECT last_insert_rowid()'
end

-------------------------------------------------------------------------------
-- Builds the UPDATE Query
-- @param   'id' (number):  id of the existing data
-- @param   'parameters' (table):  table with name of parameter and value
-- @return  (string): UPDATE query with all parameters
-------------------------------------------------------------------------------
function updateQuery(id, parameters)
    local params = {}
    for key, value in pairs(parameters) do
        if (value == "") then
            table.insert(params, key .. '= NULL')
        else
            table.insert(params, key .. '="' .. value .. '"')
        end

    end

    return 'UPDATE ' .. tableName .. ' SET ' .. table.concat(params, ", ") .. ' WHERE id= "' .. id .. '";'
end

-------------------------------------------------------------------------------
-- Builds the DELETE Query
-- @param   'id' (number):  id of the existing data
-- @return  (string): DELETE query with the id condition
-------------------------------------------------------------------------------
function deleteQuery(id)
    return 'DELETE FROM ' .. tableName .. ' WHERE id = "' .. id .. '"'
end

-------------------------------------------------------------------------------
--|                           CRUD Operations                               |--
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Creates a new resource in the database
-- @param   'parameters' (table):  table with name of parameter and value
-- @return  (string): ID of the created resource
-------------------------------------------------------------------------------
function createData(parameters)
    local db = sqlite(dbPath, "RW")
    local qry = db << insertQuery(parameters)
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
function readData(id)
    local db = sqlite(dbPath, "RW")
    local qry = db << selectIDQuery(id)
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
function readAllData(parameters)
    local db = sqlite(dbPath, "RW")
    local qry

    if (#parameters ~= 0) then
        qry = db << selectConditionQuery(andOperator(parameters))
    else
        qry = db << selectAllQuery()
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
function updateData(id, parameters)
    local db = sqlite(dbPath, "RW")
    local qry = db << updateQuery(id, parameters)
    local row = qry()

    qry =~ qry;
    db =~ db;
end

-------------------------------------------------------------------------------
-- Deletes a resource from the database
-- @param   'id' (table): ID of the resource
-------------------------------------------------------------------------------
function deleteData(id)
    local db = sqlite(dbPath, "RW")
    local qry = db << deleteQuery(id)
    local row = qry()

    qry =~ qry;
    db =~ db;
end
