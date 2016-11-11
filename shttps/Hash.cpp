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

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <openssl/evp.h>

#include "Hash.h"

using namespace std;

static const char __file__[] = __FILE__;

namespace shttps {

    Hash::Hash(HashType type) {
        EVP_MD_CTX* context = EVP_MD_CTX_create();
        if (context == NULL) {
            throw SipiError(__file__, __LINE__, "EVP_MD_CTX_create failed!");
        }
        int status;
        switch (type) {
            case none: {
                status = EVP_DigestInit_ex(context, EVP_md5(), NULL);
                break;
            }
            case md5: {
                status = EVP_DigestInit_ex(context, EVP_md5(), NULL);
                break;
            }
            case sha1: {
                status = EVP_DigestInit_ex(context, EVP_sha1(), NULL);
                break;
            }
            case sha256: {
                status = EVP_DigestInit_ex(context, EVP_sha256(), NULL);
                break;
            }
            case sha384: {
                status = EVP_DigestInit_ex(context, EVP_sha384(), NULL);
                break;
            }
            case sha512: {
                status = EVP_DigestInit_ex(context, EVP_sha512(), NULL);
                break;
            }
        }
        if (!status) {
            EVP_MD_CTX_destroy(context);
            throw SipiError(__file__, __LINE__, "EVP_DigestInit_ex failed!");
        }
    }

    Hash::~Hash() {
        EVP_MD_CTX_destroy(context);
    }

    bool Hash::add_data(const void *data, size_t len) {
        return EVP_DigestUpdate(context, data, len);
    }

    friend istream &operator>> (istream  &input, Hash &h) {
        char buffer[4096];
        int i = 0;
        while (input.good() && (i < 4096)) {
            buffer[i++] = input.get();
        }
        EVP_DigestUpdate(context, buffer, i);
        return input;
      }

    string Hash::hash(void) {
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int lengthOfHash = 0;
        string hashstr;
        if (EVP_DigestFinal_ex(context, hash, &lengthOfHash)) {
            std::stringstream ss;
            for (unsigned int i = 0; i < lengthOfHash; ++i) {
                ss << std::hex << std::setw(2) << std::setfill('0') << (int) hash[i];
            }
            hashstr = ss.str();
        }
        return hashstr;
    }
/*
    string Hash::hash(const char *data, size_t len, HashType type) {
        string hashed;
        EVP_MD_CTX* context = EVP_MD_CTX_create();

        if(context != NULL) {
            int status;
            switch (type) {
                case none: {
                    status = EVP_DigestInit_ex(context, EVP_md5(), NULL);
                    break;
                }
                case md5: {
                    status = EVP_DigestInit_ex(context, EVP_md5(), NULL);
                    break;
                }
                case sha1: {
                    status = EVP_DigestInit_ex(context, EVP_sha1(), NULL);
                    break;
                }
                case sha256: {
                    status = EVP_DigestInit_ex(context, EVP_sha256(), NULL);
                    break;
                }
                case sha384: {
                    status = EVP_DigestInit_ex(context, EVP_sha384(), NULL);
                    break;
                }
                case sha512: {
                    status = EVP_DigestInit_ex(context, EVP_sha512(), NULL);
                    break;
                }
            }
            if(status) {
                if (EVP_DigestUpdate(context, (void *) data, len)) {
                    unsigned char hash[EVP_MAX_MD_SIZE];
                    unsigned int lengthOfHash = 0;
                    if (EVP_DigestFinal_ex(context, hash, &lengthOfHash)) {
                        std::stringstream ss;
                        for (unsigned int i = 0; i < lengthOfHash; ++i) {
                            ss << std::hex << std::setw(2) << std::setfill('0') << (int) hash[i];
                        }
                        hashed = ss.str();
                    }
                }
            }
            EVP_MD_CTX_destroy(context);
        }
        return hashed;
    }
    */
}
