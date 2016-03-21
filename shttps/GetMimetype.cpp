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
#include "GetMimetype.h"
#include "Error.h"

#include "magic.h"

using namespace std;

static const char __file__[] = __FILE__;

namespace shttps {

    pair<string,string> GetMimetype::getMimetype(const string &fpath)
    {
        magic_t handle;
        if ((handle = magic_open(MAGIC_MIME | MAGIC_PRESERVE_ATIME)) == NULL) {
            throw Error(__file__, __LINE__, magic_error(handle));
        }

        if (magic_load(handle, NULL) != 0) {
            throw Error(__file__, __LINE__, magic_error(handle));
        }
        const char *result = magic_file(handle, fpath.c_str());

        string mimestr = result;

        string mimetype;
        string charset;

        size_t pos = mimestr.find(';');
        if (pos != string::npos) {
            mimetype = mimestr.substr(0, pos);
            charset = mimestr.substr(pos + 1);
        }
        else {
            mimetype = mimestr;
        }
        return make_pair(mimetype, charset);
    }

}
