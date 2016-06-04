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
 *//*!
 *
 * This file implements access to the PHP session variables
 *
 * In out case in order to access the old PHP-based Salsah database, we need to access the
 * PHP session variables. This needed some reverse engineering... The name of the file storing
 * the session variable can be determined from the session cookie.
 */
#include <fstream>
#include <exception>

namespace Sipi {

   /*!
    * \class PhpSession
    *
    * This class is Salsah specific and is not used in other environments! It implements reading
    * the session data from a PHP session using cookies. This class could probably easily be modified
    * to read any PHP session data.
    */
    class PhpSession {
    private:
        std::ifstream *inf; //>! Open input stream for the PHP session file
        std::string lang;   //>! Stores the session language
        int lang_id;
        int user_id;
        int active_project;
    public:
       /*!
        * Empty constructor needed to create an "empty variable"
        */
        inline PhpSession() {};

       /*!
        * Constructor of PhpSession class
        *
        * \param inf_p Pointer to open ifstream of the PHP session file storing the session variables
        */
        PhpSession(std::ifstream *inf_p);

       /*!
        * Returns the language ID (Salsah specific)
        */
        inline int getLangId(void) { return lang_id; }

       /*!
        * Returns the user ID (Salsah specific)
        */
        inline int getUserId(void) { return user_id; }

       /*!
        * Returns the active project ID (Salsah specific)
        */
        inline int getActiveProject(void) { return active_project; }
    };

}
