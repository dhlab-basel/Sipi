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
#ifndef __template_h
#define __template_h

#include <string>
#include <unordered_map>

/*!
 * \namespace Sipi Is used for all Sipi things.
 */
namespace Sipi {

    class Template {
    private:
        std::string templatestr;
        std::unordered_map <std::string, std::string> values;

    public:
        Template(std::string &template_p) { templatestr = template_p; };

        void value(const std::string &name, const std::string &sval);

        void value(const std::string &name, const char *cval);

        void value(const std::string &name, int ival);

        void value(const std::string &name, float fval);

        std::string get(void);
    };

}

#endif
