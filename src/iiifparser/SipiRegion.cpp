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
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>

#include <stdio.h>
#include <string.h>

#include "SipiError.h"
#include "SipiRegion.h"

static const char __file__[] = __FILE__;

namespace Sipi {

    SipiRegion::SipiRegion(std::string str) {
        int n;
        if (str.empty() || (str == "full")) {
            coord_type = FULL;
            rx = 0.;
            ry = 0.;
            rw = 0.;
            rh = 0.;
            canonical_ok = true; // "full" is a canonical value
        }
        else if (str.find("pct:") != std::string::npos) {
            coord_type = SipiRegion::PERCENTS;
            std::string tmpstr = str.substr(4);
            n = sscanf(tmpstr.c_str(), "%f,%f,%f,%f", &rx, &ry, &rw, &rh);
            if (n != 4) {
                throw SipiError(__file__, __LINE__, "IIIF Error reading Region parameter  \"" + str + "\"");
            }
            canonical_ok = false;
        }
        else {
            coord_type = SipiRegion::COORDS;
            n = sscanf(str.c_str(), "%f,%f,%f,%f", &rx, &ry, &rw, &rh);
            if (n != 4) {
                throw SipiError(__file__, __LINE__, "IIIF Error reading Region parameter  \"" + str + "\"");
            }
            canonical_ok = false;
        }
    }
    //-------------------------------------------------------------------------

    SipiRegion::CoordType SipiRegion::crop_coords(size_t nx, size_t ny, int &p_x, int &p_y, size_t &p_w, size_t &p_h) {
        switch (coord_type) {
            case COORDS: {
                x = floor(rx + 0.5);
                y = floor(ry + 0.5);
                w = floor(rw + 0.5);
                h = floor(rh + 0.5);
                break;
            }
            case PERCENTS: {
                x = floor((rx*nx/100.) + 0.5);
                y = floor((ry*ny/100.) + 0.5);
                w = floor((rw*nx/100.) + 0.5);
                h = floor((rh*ny/100.) + 0.5);
                break;
            }
            case FULL: {
                x = 0;
                y = 0;
                w = nx;
                h = ny;
                break;
            }
        }

        if (x < 0) {
            w += x;
            x = 0;
        }
        else if (x >= nx) {
            std::stringstream msg;
            msg << "Invalid croping region outside of image (x=" << x << " nx=" << nx << ")";
            throw SipiError(__file__, __LINE__, msg.str());
        }
        if (y < 0) {
            h += y;
            y = 0;
        }
        else if (y >= ny) {
            std::stringstream msg;
            msg << "Invalid croping region outside of image (y=" << y << " ny=" << ny << ")";
            throw SipiError(__file__, __LINE__, msg.str());
        }

        if (w == 0) {
            w = nx - x;
        }
        else if ((x + w) > nx) {
            w = nx - x;
        }
        if (h == 0) {
            h = ny - y;
        }
        else if ((y + h) > ny) {
            h = ny - y;
        }
        if ((w < 0) || (h < 0)) {
            std::string msg = "Invalid croping region with zero or negative dimension (w x h):" + std::to_string(w) + " " + std::to_string(h);
            throw SipiError(__file__, __LINE__, msg);
        }

        p_x = x;
        p_y = y;
        p_w = w;
        p_h = h;

        canonical_ok = true;

        return coord_type;
    }
    //-------------------------------------------------------------------------


    void SipiRegion::canonical(char *buf, int buflen) {
        if (!canonical_ok && (coord_type != FULL)) {
            std::string msg = "Canonical coordinates not determined";
            throw SipiError(__file__, __LINE__, msg);
        }
        switch (coord_type) {
            case FULL: {
                (void) snprintf (buf, buflen, "full");
                break;
            }
            case COORDS:
            case PERCENTS: {
                (void) snprintf(buf, buflen, "%d,%d,%ld,%ld", x, y, w, h);
                break;
            }
        }
    }
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // Output to stdout for debugging etc.
    //
    std::ostream &operator<< (std::ostream &outstr, const SipiRegion &rhs) {
        outstr << "IIIF-Server Region:";
        outstr << "  Coordinate type: " << rhs.coord_type;
        outstr
            << " | rx = " << std::to_string(rhs.rx)
            << " | ry = " << std::to_string(rhs.ry)
            << " | rw = " << std::to_string(rhs.rw)
            << " | rh = " << std::to_string(rhs.rh);
        return outstr;
    }
    //-------------------------------------------------------------------------

}
