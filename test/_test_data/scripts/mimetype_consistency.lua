--
-- Copyright © 2016 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
-- Ivan Subotic, Tobias Schweizer, André Kilchenmann, and André Fatton.
-- This file is part of Sipi.
-- Sipi is free software: you can redistribute it and/or modify
-- it under the terms of the GNU Affero General Public License as published
-- by the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
-- Sipi is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
-- Additional permission under GNU AGPL version 3 section 7:
-- If you modify this Program, or any covered work, by linking or combining
-- it with Kakadu (or a modified version of that library), containing parts
-- covered by the terms of the Kakadu Software Licence, the licensors of this
-- Program grant you additional permission to convey the resulting work.
-- See the GNU Affero General Public License for more details.
-- You should have received a copy of the GNU Affero General Public
-- License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.

require "send_response"

local mimetype_test_data = {
    {
        filepath = config.imgroot .. '/unit/' .. 'lena512.tif',
        mimetype = "image/tiff",
        filename = "lena512.tif"
    },
    {
        filepath = config.imgroot .. '/unit/' .. 'CV+Pub_LukasRosenthaler_p3.jpg',
        mimetype = "image/jpeg",
        filename = "CV+Pub_LukasRosenthaler.jpg"
    },
    {
        filepath = config.imgroot .. '/unit/' .. 'CV+Pub_LukasRosenthaler.pdf',
        mimetype = "application/pdf",
        filename = "CV+Pub_LukasRosenthaler.pdf"
    }
}

result = {}

for i, test_data_item in ipairs(mimetype_test_data) do

    local success, check
    success, check = server.mimetype_consistency(test_data_item.filepath, test_data_item.mimetype, test_data_item.filename)

    if not success then
        server.send_error(500, 'server.mimetype_consistency failed')
        return false
    end

    if not check then
        server.sendStatus(500)
        return -1
    end

    table.insert(result, { test_data_item, "OK" })

end

send_success(result)