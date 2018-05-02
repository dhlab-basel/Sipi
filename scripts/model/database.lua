--
-- Created by IntelliJ IDEA.
-- User: vijeinath
-- Date: 02.05.18
-- Time: 12:14
-- To change this template use File | Settings | File Templates.
--

dbPath = "testDB/testData.db"

----------------- Operator Builders -----------------

function equal(name, value)
    return name .. "='" .. value .. "'"
end

function notEqual(name, value)
    return name .. "!='" .. value .. "'"
end

function like(name, value)
    return name ..' LIKE "%' .. value .. '%"'
end

function notLike(name, value)
    return name ..' NOT LIKE "%' .. value .. '%"'
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

----------------- Query Builders -----------------

function selectAllQuery()
    return 'SELECT * FROM pdfObject'
end

function selectIDQuery(id)
    return 'SELECT * FROM pdfObject WHERE id = "'.. id .. '"'
end

function selectConditionQuery(parameters)
    return 'SELECT * FROM pdfObject WHERE ' .. parameters
end

function insertQuery(title, date)
    return 'INSERT INTO pdfObject (title, date) values (("'.. title ..'"), ("'.. date ..'"));'
end

function lastInsertedQuery()
    return 'SELECT last_insert_rowid()'
end

function updateQuery(id, title, date)
    return 'UPDATE pdfObject SET title="'.. title ..'", date="'.. date ..'" WHERE id= "'.. id .. '";'
end

function deleteQuery(id)
    return 'DELETE FROM pdfObject WHERE id = "'.. id .. '"'
end

----------------- CRUD Operations -----------------

function createData(element)
    local db = sqlite(dbPath, "RW")
    local qry = db << insertQuery(element["title"], element["date"])
    local row = qry()

    qry = db << lastInsertedQuery()
    row = qry()
    --    print(table.concat(row, ", "))

    qry = ~qry;
    db = ~db;

    return row[0]
end

function readData(id)
    local db = sqlite(dbPath, "RW")
    local qry = db << selectIDQuery(id)
    local row = qry()
    local element

    if (row ~= nil) then
        -- ACHTUNG DIES IST LUA-SYNTAKTISCH FALSCH
        element = {}
        element["id"] = row[0]
        element["title"] = row[1]
        element["date"] = row[2]
    end

    qry = ~qry; -- delete query and free prepared statment
    db = ~db; -- delete the database connection

    return element
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
    local elements = {}

    while (row) do
        local element = {}
        element["id"] = row[0]
        element["title"] = row[1]
        element["date"] = row[2]
        table.insert(elements, element)
        row = qry()
    end

    qry = ~qry;
    db = ~db;

    return elements
end

function updateData(id, element)
    local db = sqlite(dbPath, "RW")
    local qry = db << updateQuery(id, element["title"], element["date"])
    local row = qry()

    qry = ~qry;
    db = ~db;
end

function deleteData(id)
    local db = sqlite(dbPath, "RW")
    local qry = db << deleteQuery(id)
    local row = qry()

    qry = ~qry;
    db = ~db;
end
