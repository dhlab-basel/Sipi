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
 * This file implements the virtual abstract class which implements the image file I/O.
 */
#ifndef __defined_xmp_h
#define __defined_xmp_h

#include <string>
#include <mutex>
#include <exiv2/xmp_exiv2.hpp> //!< Import xmp from the exiv2 library!
#include <exiv2/error.hpp>

namespace Sipi {

    typedef struct {
        std::mutex lock;
    } XmpMutex;

    extern XmpMutex xmp_mutex;

    extern void xmplock_func(void *pLockData, bool lockUnlock);

    /*!
    * This class handles XMP metadata. It uses the Exiv2 library
    */
    class SipiXmp {
    private:
        Exiv2::XmpData xmpData; //!< Private member variable holding the exiv2 XMP data
        std::string __xmpstr;

    public:
        /*!
         * Constructor
         *
         * \param[in] xmp A std::string containing RDF/XML with XMP data
         */
        SipiXmp(const std::string &xmp);

        /*!
         * Constructor
         *
         * \param[in] xmp A C-string (char *)containing RDF/XML with XMP data
         */
        SipiXmp(const char *xmp);

        /*!
         * Constructor
         *
         * \param[in] xmp A string containing RDF/XML with XMP data
         * \param[in] len Length of the string
         */
        SipiXmp(const char *xmp, int len);


        /*!
         * Destructor
         */
        ~SipiXmp();


        /*!
        * Returns the bytes of the RDF/XML data
        * \param[out] len Length of the data on bytes
        * \returns Chunk of chars holding the xmp data
        */
        char *xmpBytes(unsigned int &len);

        /*!
         * Returns the bytes of the RDF/XML data as std::string
         *
         * @return String holding the xmp data
         */
        std::string xmpBytes(void);

        /*!
         * The overloaded << operator which is used to write the xmp formatted to the outstream
         *
         * \param[in] lhs The output stream
         * \param[in] rhs Reference to an instance of a SipiXmp
         * \returns Returns ostream object
         */
        friend std::ostream &operator<<(std::ostream &lhs, const SipiXmp &rhs);

    };

}

#endif
