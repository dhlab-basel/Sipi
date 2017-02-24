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
/*!
 * SipiImage is the core object of dealing with images within the Sipi package
 * The SipiImage object holds all the information about an image and offers the methods
 * to read, write and modify images. Reading and writing is supported in several standard formats
 * such as TIFF, J2k, PNG etc.
 */
#ifndef __shttps_hash_h
#define __shttps_hash_h

#include <iostream>

#include <openssl/evp.h>


namespace shttps {

    typedef enum {
        none = 0, md5 = 1, sha1 = 2, sha256 = 3, sha384 = 4, sha512 = 5
    } HashType;

    /*!
     * \brief Hash class which implements a variety of checksum schemes
     * \author Lukas Rosenthaler
     * \version 1.0
     * \date 2016-11-11
     */
    class Hash {
    private:
        EVP_MD_CTX *context;

    public:
        /*!
        * Constructor of a Hash instance
        *
        * \param[in] type Hash/checksum method to use (see HashType)
        */
        Hash(HashType type);

        /*!
        * Destructor which cleans up everything
        */
        ~Hash();

        /*!
        * Adds data to the hash
        *
        * \param[in] data Pointer to data
        * \param[in] len Length of data in bytes
        *
        * \returns true in case of success, false if the data couln't be processed
        */
        bool add_data(const void *data, size_t len);

        /*!
        * Calculate the checksum of a fileType_string. THe method uses directly
        * the unix system calls open, read and write and does the buffering
        * internally.
        *
        * \param[in] path Path to the file
        * \param[in] buflen Internal buffer for reading the file
        *
        * \returns true in case of success, false if the data couln't be processed
        */
        bool hash_of_file(const std::string &path, size_t buflen = 16 * 1024);

        /*!
        * Adds data to the has from a input stream. It reads the stream until
        * an eof is encountered!
        *
        * \code{.cpp}
        * stringstream strstr;
        * ss << "Waseliwas " << a_number << " ist das ?";
        * shttps::Hash h(shttp::HashType:md5);
        * strstr >> h;
        * std::string checksum = h.hash();
        * \endcode
        */
        friend std::istream &operator>>(std::istream &input, Hash &h);

        /*!
        * Calculate and return the has value as string
        *
        * \returns Returns the has value as string
        */
        std::string hash(void);
    };

}

#endif
