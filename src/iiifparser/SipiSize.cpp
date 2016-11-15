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
 */#include <assert.h>
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

#include "shttps/spdlog/spdlog.h"

#include "shttps/Global.h"
#include "SipiError.h"
#include "SipiSize.h"

using namespace std;

static const char __file__[] = __FILE__;

namespace Sipi {

    SipiSize::SipiSize(string str) {
        nx = ny = 0;
        percent = 0.;
        canonical_ok = false;

        int n;
        if (str.empty() || (str == "full")) {
            size_type = SizeType::FULL;
        }
        else if (str.find("pct") != string::npos) {
            size_type = SizeType::PERCENTS;
            string tmpstr = str.substr(4);
            n = sscanf(tmpstr.c_str(), "%f", &percent);
            if (n != 1) {
                throw SipiError(__file__, __LINE__, "IIIF Error reading Size parameter  \"" + str + "\": Expected \"pct:float\" !");
            }
            if (percent < 0.0) percent = 1.0;
            if (percent > 100.0) percent = 100.0;
        }
        else if (str.find("red") != string::npos) {
            size_type = SizeType::REDUCE;
            string tmpstr = str.substr(4);
            n = sscanf(tmpstr.c_str(), "%d", &reduce);
            if (n != 1) {
                throw SipiError(__file__, __LINE__, "IIIF Error reading Size parameter  \"" + str + "\": Expected \"red:int\" !");
            }
            if (reduce < 0) percent = 0;
        }
        else {
            if (str[0] == '!') {
                size_type = SizeType::MAXDIM;
                n = sscanf(str.c_str(), "!%d,%d", &nx, &ny);
            }
            else {
                n = sscanf(str.c_str(), "%d,%d", &nx, &ny);
                if (n == 2) {
                    size_type = SizeType::PIXELS_XY;
                }
                else if (n == 1) {
                    size_type = SizeType::PIXELS_X;
                }
                else {
                    n = sscanf(str.c_str(), ",%d", &ny);
                    if (n != 1) {
                        throw SipiError(__file__, __LINE__, "IIIF Error reading Size parameter  \"" + str + "\" !");
                    }
                    size_type = SizeType::PIXELS_Y;
               }
            }
        }
    };
    //-------------------------------------------------------------------------

    SipiSize::SipiSize(int nx_p, int ny_p, bool maxdim) {
        canonical_ok = false;
        if (nx_p <= 0) {
            ny = ny_p;
            size_type = SizeType::PIXELS_Y;
        }
        else if (ny_p <= 0) {
            nx = nx_p;
            size_type = SizeType::PIXELS_X;
        }
        else if ((nx_p <= 0) && (ny_p <= 0)) {
            throw SipiError(__file__, __LINE__, "Invalid Size parameter! Both dimensions <= 0!");
        }
        else {
            nx = nx_p;
            ny = ny_p;
            if (maxdim) {
                size_type = SizeType::MAXDIM;
            }
            else {
                size_type = SizeType::PIXELS_XY;
            }
        }
    }
    //-------------------------------------------------------------------------

    bool SipiSize::operator >(const SipiSize &s)
    {
        if (!canonical_ok) {
            string msg = "Final size not yet determined!";
            throw SipiError(__file__, __LINE__, msg);
        }
        return ((w > s.w) || (h > s.h));
    }
    //-------------------------------------------------------------------------

    bool SipiSize::operator >=(const SipiSize &s)
    {
        if (!canonical_ok) {
            string msg = "Final size not yet determined!";
            throw SipiError(__file__, __LINE__, msg);
        }
        return ((w >= s.w) || (h >= s.h));
    }
    //-------------------------------------------------------------------------

    bool SipiSize::operator <(const SipiSize &s)
    {
        if (!canonical_ok) {
            string msg = "Final size not yet determined!";
            throw SipiError(__file__, __LINE__, msg);
        }
        return ((w < s.w) && (h < s.h));
    }
    //-------------------------------------------------------------------------

    bool SipiSize::operator <=(const SipiSize &s)
    {
        if (!canonical_ok) {
            string msg = "Final size not yet determined!";
            throw SipiError(__file__, __LINE__, msg);
        }
        return ((w <= s.w) && (h <= s.h));
    }
    //-------------------------------------------------------------------------

    SipiSize::SizeType SipiSize::get_size(int img_w, int img_h, int &w_p, int &h_p, int &reduce_p, bool &redonly_p) {
        redonly = false;

        auto logger = spdlog::get(shttps::loggername);

        switch (size_type) {
            case SipiSize::PIXELS_XY: {
                //
                // first we check how far the new image width and image hight can be reached by a reduce factor
                //
                int sf_w = 1;
                int reduce_w = 0;
                bool exact_match_w = true;
                w = ceil((float) img_w / (float) sf_w);
                while (w > nx) {
                    sf_w *= 2;
                    w = ceil((float) img_w / (float) sf_w);
                    reduce_w++;
                }
                if (w != nx) {
                    // we do not have an exact match. Go back one level with reduce
                    exact_match_w = false;
                    sf_w /= 2;
                    reduce_w--;
                }

                int sf_h = 1;
                int reduce_h = 0;
                bool exact_match_h = true;
                h = ceil((float) img_h / (float) sf_h);
                while (h > ny) {
                    sf_h *= 2;
                    h = ceil((float) img_h / (float) sf_h);
                    reduce_h++;
                }
                if (h != ny) {
                    cerr << "no exact match h=" << h << " ny=" << ny << endl;
                    // we do not have an exact match. Go back one level with reduce
                    exact_match_h = false;
                    sf_h /= 2;
                    reduce_h--;
                }

                if (exact_match_w && exact_match_h && (reduce_w == reduce_h)) {
                    reduce = reduce_w;
                    redonly = true;
                }
                else {
                    reduce = reduce_w < reduce_h ? reduce_w : reduce_h; // min()
                    redonly = false;
                }

                w = nx;
                h = ny;
                break;
            }
            case SipiSize::PIXELS_X: {
                //
                // first we check how far the new image width can be reached by a reduce factor
                //
                int sf_w = 1;
                int reduce_w = 0;
                bool exact_match_w = true;
                w = ceil((float) img_w / (float) sf_w);
                while (w > nx) {
                    sf_w *= 2;
                    w = ceil((float) img_w / (float) sf_w);
                    reduce_w++;
                }
                if (w != nx) {
                    // we do not have an exact match. Go back one level with reduce
                    exact_match_w = false;
                    sf_w /= 2;
                    reduce_w--;
                }

                w = nx;
                reduce = reduce_w;
                redonly = exact_match_w; // if exact_match, then reduce only!
                if (exact_match_w) {
                    h = ceil((float) img_h / (float) sf_w);
                }
                else {
                    h = ceil((float) (img_h*nx) / (float) img_w);
                }
                break;
            }
            case SipiSize::PIXELS_Y: {
                //
                // first we check how far the new image hight can be reched by a reduce factor
                //
                int sf_h = 1;
                int reduce_h = 0;
                bool exact_match_h = true;
                h = ceil((float) img_h / (float) sf_h);
                while (h > ny) {
                    sf_h *= 2;
                    h = ceil((float) img_h / (float) sf_h);
                    reduce_h++;
                }
                if (h != ny) {
                    // we do not have an exact match. Go back one level with reduce
                    exact_match_h = false;
                    sf_h /= 2;
                    reduce_h--;
                }

                h = ny;
                reduce = reduce_h;
                redonly = exact_match_h; // if exact_match, then reduce only!
                if (exact_match_h) {
                    w = ceil((float) img_w / (float) sf_h);
                }
                else {
                    w = ceil((float) (img_w*ny) / (float) img_h);
                }
                break;
            }
            case SipiSize::PERCENTS: {
                w = ceil(img_w*percent / 100.F);
                h = ceil(img_h*percent / 100.F);

                reduce = 0;
                float r = 100. / percent;
                float s = 1.0;
                while (2.0*s <= r) {
                    s *= 2.0;
                    reduce++;
                }
                if (fabsf(s - r) < 1.0e-5) {
                    redonly = true;
                }
                break;
            }
            case SipiSize::REDUCE: {
                if (reduce == 0) {
                    w = img_w;
                    h = img_h;
                    redonly = true;
                    break;
                }
                int sf = 1;
                for (int i = 0; i < reduce; i++) sf *= 2;
                w = ceil((float) img_w / (float) sf);
                h = ceil((float) img_h / (float) sf);
                redonly = true;
                break;
            }
            case SipiSize::MAXDIM: {
                float fx = (float) nx / (float) img_w;
                float fy = (float) ny / (float) img_h;

                float r;
                if (fx < fy) { // scaling has to be done by fx
                    w = nx;
                    h = ceil(img_h*fx);

                    r = (float) img_w / (float) w;
                }
                else { // scaling has to be done fy
                    w = ceil(img_w*fy);
                    h = ny;

                    r = (float) img_h / (float) h;
                }

                float s = 1.0;
                reduce = 0;
                while (2.0*s <= r) {
                    s *= 2.0;
                    reduce++;
                }
                if (fabsf(s - r) < 1.0e-5) {
                    redonly = true;
                }
                break;
            }
            case SipiSize::FULL: {
                w = img_w;
                h = img_h;
                reduce = 0;
                redonly = true;
            }
        }

        logger->debug() << "get_size: img_w=" << img_w << " img_h=" << img_h << " w="
            << w << " h=" << h << " reduce=" << reduce << " reduce only=" << redonly;

        w_p = w;
        h_p = h;

        reduce_p = reduce < 0 ? 0 : reduce;
        redonly_p = redonly;

        canonical_ok = true;

        return size_type;
    }
    //-------------------------------------------------------------------------


    void SipiSize::canonical(char *buf, int buflen) {
        int n;
        if (!canonical_ok && (size_type != SipiSize::FULL)) {
            string msg = "Canonical size not determined!";
            throw SipiError(__file__, __LINE__, msg);
        }
        switch (size_type) {
            case SipiSize::PIXELS_XY: {
                n = snprintf(buf, buflen, "%d,%d", w, h);
                break;
            }
            case SipiSize::PIXELS_X: {
                n = snprintf(buf, buflen, "%d,", w);
                break;
            }
            case SipiSize::PIXELS_Y: {
                n = snprintf(buf, buflen, "%d,", w);
                break;
            }
            case SipiSize::MAXDIM: {
                n = snprintf(buf, buflen, "%d,", w);
                break;
            }
            case SipiSize::PERCENTS: {
                n = snprintf(buf, buflen, "%d,", w);
                break;
            }
            case SipiSize::REDUCE: {
                n = snprintf(buf, buflen, "%d,", w);
                break;
            }
            case SipiSize::FULL: {
                n = snprintf(buf, buflen, "full"); // replace with "max" for version 3.0 of iiif
            }
        }
        if ((n < 0) || (n >= buflen)) {
            throw SipiError(__file__, __LINE__, "Error creating size string!");
        }
    }
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // Output to stdout for debugging etc.
    //
    std::ostream &operator<< (ostream &outstr, const SipiSize &rhs) {
        outstr << "IIIF-Server Size parameter:";
        outstr << "  Size type: " << rhs.size_type;
        outstr
            << " | percent = " << to_string(rhs.percent)
            << " | nx = " << to_string(rhs.nx)
            << " | ny = " << to_string(rhs.ny);
        return outstr;
    };
    //-------------------------------------------------------------------------

}
