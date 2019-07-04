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
#ifndef __defined_iptc_h
#define __defined_iptc_h

#include <string>
#include <vector>
#include <exiv2/iptc.hpp>

namespace Sipi {

    /*!
     * Handles IPTC data based on the exiv2 library
     */
    class SipiIptc {
    private:
        Exiv2::IptcData iptcData; //!< Private member variable holding the exiv2 IPTC object

    public:
        /*!
         * Constructor
         *
         * \param[in] Buffer containing the IPTC data in native format
         * \param[in] Length of the buffer
         */
        SipiIptc(const unsigned char *iptc, unsigned int len);

        /*!
         * Destructor
         */
        ~SipiIptc();

        /*!
        * Returns the bytes of the IPTC data. The buffer must be
        * deleted by the caller after it is no longer used!
        * \param[out] len Length of the data in bytes
        * \returns Chunk of chars holding the IPTC data
        */
        unsigned char *iptcBytes(unsigned int &len);

        /*!
         * Returns the bytes of the IPTC data as std::vector
         * @return IPTC bytes as std::vector
         */
        std::vector<unsigned char> iptcBytes(void);

        /*!
         * The overloaded << operator which is used to write the IPTC data formatted to the outstream
         *
         * \param[in] lhs The output stream
         * \param[in] rhs Reference to an instance of a SipiIptc
         * \returns Returns ostream object
         */
        friend std::ostream &operator<<(std::ostream &lhs, SipiIptc &rhs);

    };

}

#endif
