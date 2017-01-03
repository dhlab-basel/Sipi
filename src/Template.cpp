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
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <stdio.h>
#include <unistd.h>

#include "SipiError.h"
#include "Template.h"

static const char __file__[] = __FILE__;

namespace Sipi {

    void Template::value(const std::string &name, const string &sval) {
        values[name] = sval;
    }

    void Template::value(const std::string &name, const char *cval) {
        values[name] = string(cval);
    }

    void Template::value(const std::string &name, int ival) {
        values[name] = to_string(ival);
    }

    void Template::value(const std::string &name, float fval) {
        values[name] = to_string(fval);
    }

    std::string Template::get(void) {
        std::string result;
        size_t pos, old_pos = 0, epos = 0;
        while ((pos = templatestr.find("{{", old_pos)) != string::npos) {
            // we found somtheing to replace
            result += templatestr.substr(epos, pos - epos); // copy and add string up to token
            epos = templatestr.find("}}", pos + 2);
            if (epos == string::npos) throw SipiError(__file__, __LINE__, "Error in template!");
            pos += 2; // sckip the {{
            string name = templatestr.substr(pos, epos - pos);
            result += values[name];
            epos += 2;
            old_pos = pos;
        }
        result += templatestr.substr(epos);
        return result;
    }

}
