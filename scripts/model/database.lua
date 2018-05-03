--
-- Created by IntelliJ IDEA.
-- User: vijeinath
-- Date: 02.05.18
-- Time: 12:14
-- To change this template use File | Settings | File Templates.
--

dbPath = "testDB/testData.db"
tableName = "resource"

-------------------------- Operator Builders ----------------------------------
-------------------------------------------------------------------------------
-- Builds the EQUAL statement
-- @param   'name' (string):  name of the parameter
-- @param   'value' (string):  value of the parameter
-- @return  (string): statement with EQUAL with the parameter
-------------------------------------------------------------------------------
function equal(name, value)
    return name .. "='" .. value .. "'"
end

-------------------------------------------------------------------------------
-- Builds the not EQUAL statement
-- @param   'name' (string):  name of the parameter
-- @param   'value' (string):  value of the parameter
-- @return  (string): statement with EQUAL with the parameter
-------------------------------------------------------------------------------
function notEqual(name, value)
    return name .. "!='" .. value .. "'"
end

function like(name, value)
    return name .. ' LIKE "%' .. value .. '%"'
end

function notLike(name, value)
    return name .. ' NOT LIKE "%' .. value .. '%"'
end

function exists(name)
    return name .. ' IS NOT NULL '
end

function notExists(name)
    return name .. ' IS NULL'
end

function greaterThan(name, value)
    return name .. ' > "' .. value .. '"'
end

function greaterThanEqual(name, value)
    return name .. ' >= "' .. value .. '"'
end

function lessThan(name, value)
    return name .. ' < "' .. value .. '"'
end

function lessThanEqual(name, value)
    return name .. ' <= "' .. value .. '"'
end

function betweenDates(date1, date2)
    return 'BETWEEN "' .. date1 .. '" AND "' .. date2 .. '"'
end

function andOperator(parameters)
    return table.concat(parameters, " AND ")
end

function orOperator(parameters)
    return table.concat(parameters, " OR ")
end

---------------------------- Query Builders -----------------------------------
-------------------------------------------------------------------------------
-- Builds the SELECT all Query
-- @return  (string): select query
-------------------------------------------------------------------------------
function selectAllQuery()
    return 'SELECT * FROM ' .. tableName
end

function selectIDQuery(id)
    return 'SELECT * FROM ' .. tableName .. ' WHERE ' .. equal("id", id)
end

function selectConditionQuery(conditions)
    return 'SELECT * FROM ' .. tableName .. ' WHERE ' .. conditions
end

function insertQuery_v1(parameters)
    -- NULL is not taken account of
    return 'INSERT INTO ' .. tableName .. ' (title, creator, subject, description, publisher, contributor, date, type, format, identifier, source, language, relation, coverage, rights) values (("' .. parameters["title"] .. '"), ("' .. parameters["creator"] .. '"), ("' .. parameters["subject"] .. '"), ("' .. parameters["description"] .. '"), ("' .. parameters["publisher"] .. '"), ("' .. parameters["contributor"] .. '"), ("' .. parameters["date"] .. '"), ("' .. parameters["type"] .. '"), ("' .. parameters["format"] .. '"), ("' .. parameters["identifier"] .. '"), ("' .. parameters["source"] .. '"), ("' .. parameters["language"] .. '"), ("' .. parameters["relation"] .. '"), ("' .. parameters["coverage"] .. '"), ("' .. parameters["rights"] .. '"));'
end

function insertQuery_v2(parameters)
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

function lastInsertedQuery()
    return 'SELECT last_insert_rowid()'
end

function updateQuery_v1(id, parameters)
    -- NULL is not taken account of
    return 'UPDATE ' .. tableName .. ' SET title="' .. parameters["title"] .. '", creator="' .. parameters["creator"] .. '", subject="' .. parameters["subject"] .. '", description="' .. parameters["description"] .. '", publisher="' .. parameters["publisher"] .. '", contributor="' .. parameters["contributor"] .. '", date="' .. parameters["date"] .. '", type="' .. parameters["type"] .. '", format="' .. parameters["format"] .. '", identifier="' .. parameters["identifier"] .. '", source="' .. parameters["source"] .. '", language="' .. parameters["language"] .. '", relation="' .. parameters["relation"] .. '", coverage="' .. parameters["coverage"] .. '", rights="' .. parameters["rights"] .. '" WHERE id= "' .. id .. '";'
end

function updateQuery_v2(id, parameters)
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

function deleteQuery(id)
    return 'DELETE FROM ' .. tableName .. ' WHERE id = "' .. id .. '"'
end

----------------- CRUD Operations -----------------
function createData(parameters)
    local db = sqlite(dbPath, "RW")
    local qry = db << insertQuery_v2(parameters)
    local row = qry()

    qry = db << lastInsertedQuery()
    row = qry()
    --    print(table.concat(row, ", "))

    qry =~ qry;
    db =~ db;

    return row[0]
end

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

function updateData(id, parameters)
    local db = sqlite(dbPath, "RW")
    local qry = db << updateQuery_v2(id, parameters)
    local row = qry()

    qry =~ qry;
    db =~ db;
end

function deleteData(id)
    local db = sqlite(dbPath, "RW")
    local qry = db << deleteQuery(id)
    local row = qry()

    qry =~ qry;
    db =~ db;
end
