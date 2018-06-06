-------------------------------------------------------------------------------
--|                         Operator Builders                               |--
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Builds the EQUAL statement which is case insensitive
-- @param   'name' (string):  name of the parameter
-- @param   'value' (string):  value of the parameter
-- @return  (string): EQUAL statement involving the parameter
-------------------------------------------------------------------------------
function equal(name, value)
    return name .. "='" .. value .. "' COLLATE NOCASE"
end

-------------------------------------------------------------------------------
-- Builds the not EQUAL statement which is case insensitive
-- @param   'name' (string):  name of the parameter
-- @param   'value' (string):  value of the parameter
-- @return  (string): Not EQUAL statement involving the parameter
-------------------------------------------------------------------------------
function notEqual(name, value)
    return name .. "!='" .. value .. "' COLLATE NOCASE"
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
function isNotNull(name)
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
-- @param   'name' (string):  name of the parameter
-- @param   'date1' (string):  start date
-- @param   'date2' (string):  end date
-- @return  (string): BETWEEN the dates statement
-------------------------------------------------------------------------------
function betweenDates(name, date1, date2)
    return name .. ' BETWEEN "' .. date1 .. '" AND "' .. date2 .. '"'
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
-- @param   'tableName' (string):  name of the table in the database
-- @return  (string): SELECT query
-------------------------------------------------------------------------------
function selectAllQuery(tableName)
    return 'SELECT * FROM ' .. tableName
end

-------------------------------------------------------------------------------
-- Builds the SELECT id Query
-- @param   'id' (table):  id of the data
-- @param   'tableName' (string):  name of the table in the database
-- @return  (string): SELECT query with id condition
-------------------------------------------------------------------------------
function selectIDQuery(id, tableName)
    return 'SELECT * FROM ' .. tableName .. ' WHERE ' .. equal("id", id)
end

-------------------------------------------------------------------------------
-- Builds the SELECT Query
-- @param   'conditions' (string):  condition which can include serveral statements
-- @param   'tableName' (string):  name of the table in the database
-- @return  (string): SELECT query with concatinated conditions
-------------------------------------------------------------------------------
function selectConditionQuery(conditions, tableName)
    return 'SELECT * FROM ' .. tableName .. ' WHERE ' .. conditions
end

-------------------------------------------------------------------------------
-- Builds the INSERT Query
-- @param   'parameters' (table):  table with name of parameter and value
-- @param   'tableName' (string):  name of the table in the database
-- @return  (string): INSERT query with all parameters
-------------------------------------------------------------------------------
function insertQuery(parameters, tableName)
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
-- @param   'tableName' (string):  name of the table in the database
-- @return  (string): UPDATE query with all parameters
-------------------------------------------------------------------------------
function updateQuery(id, parameters, tableName)
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
-- @param   'tableName' (string):  name of the table in the database
-- @return  (string): DELETE query with the id condition
-------------------------------------------------------------------------------
function deleteQuery(id, tableName)
    return 'DELETE FROM ' .. tableName .. ' WHERE id = "' .. id .. '"'
end
