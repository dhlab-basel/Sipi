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
#ifndef __defined_essentials_h
#define __defined_essentials_h

#include <cstdlib>
#include <sstream>
#include <utility>
#include <string>


#include "shttps/Hash.h"

namespace Sipi {

    /*!
    * Small class to create a small essentaion metadata packet that can be embedded
    * within an image header. It contains
    * - original file name
    * - original mime type
    * - checksum method
    * - checksum of uncompressed pixel values
    * The class contains methods to set the fields, to serialize and deserialize
    * the data.
    */
    class SipiEssentials {
    private:
        bool _is_set;
        std::string _origname; //!< oroginal filename
        std::string _mimetype; //!< original mime type
        shttps::HashType _hash_type; //!< type of checksum
        std::string _data_chksum; //!< the checksum of pixel data

    public:
        /*!
        * Constructor for empty packet
        */
        inline SipiEssentials() {
            _hash_type = shttps::HashType::none;
            _is_set = false;
        }

        /*!
        * Constructor where all fields are passed
        *
        * \param[in] origname_p Original filename
        * \param[in] mimetype_p Original mimetype as string
        * \param[in] hash_type_p Checksumtype as defined in Hash.h (shttps::HashType)
        * \param[in] data_chksum The actual checksum of the internal image data
        */
        inline SipiEssentials(const std::string &origname_p, const std::string &mimetype_p,
                              shttps::HashType hash_type_p, const std::string &data_chksum_p) :
                _origname(origname_p),
                _mimetype(mimetype_p),
                _hash_type(hash_type_p),
                _data_chksum(data_chksum_p) { _is_set = true; }

        /*!
        * Constructor taking a serialized packet (as string)
        *
        * \param[in] datastr Serialzed metadata packet
        */
        inline SipiEssentials(const std::string &datastr) { parse(datastr); }

        /*!
        * Getter for original name
        */
        inline std::string origname(void) { return _origname; }

        /*!
        * Setter for original name
        */
        inline void origname(const std::string &origname_p) {
            _origname = origname_p;
            _is_set = true;
        }

        /*!
        * Getter for mimetype
        */
        inline std::string mimetype(void) { return _mimetype; }

        /*!
        * Setter for original name
        */
        inline void mimetype(const std::string &mimetype_p) { _mimetype = mimetype_p; }

        /*!
        * Getter for checksum type as shttps::HashType
        */
        inline shttps::HashType hash_type(void) { return _hash_type; }

        /*!
        * Getter for checksum type as string
        */
        std::string hash_type_string(void) const;

        /*!
        * Setter for checksum type
        *
        * \param[in] hash_type_p checksum type
        */
        inline void hash_type(shttps::HashType hash_type_p) { _hash_type = hash_type_p; }

        /*!
        * Setter for checksum type
        *
        * \param[in] hash_type_p checksum type
        */
        void hash_type(const std::string &hash_type_p);

        /*!
        * Getter for checksum
        */
        inline std::string data_chksum(void) { return _data_chksum; }

        /*!
        * Setter for checksum
        */
        inline void data_chksum(const std::string &data_chksum_p) { _data_chksum = data_chksum_p; }

        /*!
        * Parse a string containing a serialized metadata packet
        */
        void parse(const std::string &str);

        /*!
        * Check if essential metadata has been set.
        */
        inline bool is_set(void) { return _is_set; }

        /*!
        * String conversion operator
        */
        inline operator std::string() const {
            std::string tmpstr = _origname + "|" + _mimetype + "|" + hash_type_string() + "|" + _data_chksum;
            return tmpstr;
        }

        /*!
        * Stream output operator
        */
        inline friend std::ostream &operator<<(std::ostream &ostr, const SipiEssentials &rhs) {
            ostr << rhs._origname << "|" << rhs._mimetype << "|" << rhs.hash_type_string() << "|" << rhs._data_chksum;
            return ostr;
        }
    };
}


#endif
