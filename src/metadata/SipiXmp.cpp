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
 */#include "SipiError.h"
#include "SipiXmp.h"
#include <pthread.h>

static const char __file__[] = __FILE__;

using namespace std;

namespace Sipi {


    SipiXmp::SipiXmp(string xmp) {
        if (Exiv2::XmpParser::decode(xmpData, xmp) != 0) {
            Exiv2::XmpParser::terminate();
            throw SipiError(__file__, __LINE__, "Could not parse XMP!");
        }
        Exiv2::XmpParser::terminate();

        //xmpData["Xmp.sipi.software"] = "sipi 1.0.0";
    }
    //============================================================================

    SipiXmp::SipiXmp(const char *xmp) {
        if (Exiv2::XmpParser::decode(xmpData, xmp) != 0) {
            Exiv2::XmpParser::terminate();
            throw SipiError(__file__, __LINE__, "Could not parse XMP!");
        }
        //xmpData["Xmp.sipi.version"] = "sipi 1.0.0";
    }
    //============================================================================

    SipiXmp::SipiXmp(const char *xmp, int len) {
        char *buf = new char[len + 1];
        memcpy (buf, xmp, len);
        buf[len] = '\0';

        if (Exiv2::XmpParser::decode(xmpData, buf) != 0) {
            Exiv2::XmpParser::terminate();
            delete [] buf;
            throw SipiError(__file__, __LINE__, "Could not parse XMP!");
        }
        delete [] buf;

        //xmpData["Xmp.sipi.version"] = "sipi 1.0.0";
    }
    //============================================================================


    SipiXmp::~SipiXmp() {
        Exiv2::XmpParser::terminate();
    }
    //============================================================================


    char * SipiXmp::xmpBytes(unsigned int &len) {
        std::string xmpPacket;
        if (0 != Exiv2::XmpParser::encode(xmpPacket, xmpData)) {
            throw SipiError(__file__, __LINE__, "Failed to serialize XMP data!");
        }
        Exiv2::XmpParser::terminate();

        len = xmpPacket.size();
        char *buf = new char[len + 1];
        memcpy (buf, xmpPacket.c_str(), len);
        buf[len] = '\0';
        return buf;
    }
    //============================================================================

    ostream &operator<< (ostream &outstr, const SipiXmp &rhs) {
        for (Exiv2::XmpData::const_iterator md = rhs.xmpData.begin();
        md != rhs.xmpData.end(); ++md) {
            outstr << std::setfill(' ') << std::left
                << std::setw(44)
                << md->key() << " "
                << std::setw(9) << std::setfill(' ') << std::left
                << md->typeName() << " "
                << std::dec << std::setw(3)
                << std::setfill(' ') << std::right
                << md->count() << "  "
                << std::dec << md->value()
                << std::endl;
        }
        return outstr;
    }
    //============================================================================

}
