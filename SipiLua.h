/*
 * Copyright © 2016 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
 * Ivan Subotic, Tobias Schweizer, André Kilchenmann, and André Fatton.
 * This file is part of Sipi.
 * Sipi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Sipi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Additional permission under GNU AGPL version 3 section 7:
 * If you modify this Program, or any covered work, by linking or combining
 * it with Kakadu (or a modified version of that library) or Adobe ICC Color
 * Profiles (or a modified version of that library) or both, containing parts
 * covered by the terms of the Kakadu Software Licence or Adobe Software Licence,
 * or both, the licensors of this Program grant you additional permission
 * to convey the resulting work.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public
 * License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SIPI_SIPILUA_H
#define SIPI_SIPILUA_H

#include <iostream>
#include <vector>
#include <map>

#include "LuaServer.h"

/*!
 * This module implements a new Lua datatype, "SipiImage"
 * This datatype is used to manipulate the Sipi::Image c++ class
 * The following Lua functions are implemented:
 *
 *    img = SipiImage.new("filename")
 *    img = SipiImage.new("filename",{region=<iiif-region-string>, size=<iiif-size-string> | reduce=<integer>})
 *    dims = SipiImage.dim(img) -- returns table { nx=<nx>, ny=<ny> }
 *    SipiImage.write(img, <filepath>)
 *    SipiImage.send(img, format)
 */
namespace Sipi {

    extern void sipiGlobals(lua_State *L, shttps::Connection &conn, void *user_data);

}


#endif //SIPI_SIPILUA_H
