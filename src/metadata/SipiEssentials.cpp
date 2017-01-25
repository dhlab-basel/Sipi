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

#include <vector>

#include "shttps/Global.h"
#include "SipiEssentials.h"

static const char __file__[] = __FILE__;

namespace Sipi {

    std::string SipiEssentials::hash_type_string(void) const {
        std::string hash_type_str;
        switch (_hash_type) {
            case shttps::HashType::none:    hash_type_str = "none";   break;
            case shttps::HashType::md5:     hash_type_str = "md5";   break;
            case shttps::HashType::sha1:    hash_type_str = "sha1";   break;
            case shttps::HashType::sha256:   hash_type_str = "sha256";   break;
            case shttps::HashType::sha384:  hash_type_str = "sha384";   break;
            case shttps::HashType::sha512:  hash_type_str = "sha512";   break;
        }
        return hash_type_str;
    }

    void SipiEssentials::hash_type(const std::string &hash_type_p) {
        if (hash_type_p == "none")        _hash_type = shttps::HashType::none;
        else if (hash_type_p == "md5")    _hash_type = shttps::HashType::md5;
        else if (hash_type_p == "sha1")   _hash_type = shttps::HashType::sha1;
        else if (hash_type_p == "sha256") _hash_type = shttps::HashType::sha256;
        else if (hash_type_p == "sha384") _hash_type = shttps::HashType::sha384;
        else if (hash_type_p == "sha512") _hash_type = shttps::HashType::sha512;
        else _hash_type = shttps::HashType::none;
    }

    void SipiEssentials::parse(const std::string &str) {
        std::vector<std::string> result(4);
        shttps::explode(str, '|', result.begin());
        _origname = *result.begin();
        _mimetype = *(result.begin() + 1);
        std::string _hash_type_str = *(result.begin() + 2);
        if (_hash_type_str == "none") _hash_type = shttps::HashType::none;
        else if (_hash_type_str == "md5") _hash_type = shttps::HashType::md5;
        else if (_hash_type_str == "sha1") _hash_type = shttps::HashType::sha1;
        else if (_hash_type_str == "sha256") _hash_type = shttps::HashType::sha256;
        else if (_hash_type_str == "sha384") _hash_type = shttps::HashType::sha384;
        else if (_hash_type_str == "sha512") _hash_type = shttps::HashType::sha512;
        else _hash_type = shttps::HashType::none;
        _data_chksum = *(result.begin() + 3);
         _is_set = true;
    }
}
