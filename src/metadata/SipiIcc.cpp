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
#include "SipiIcc.h"

#include <time.h>

static const char __file__[] = __FILE__;

#include "SipiError.h"
#include "AdobeRGB1998_icc.h"
#include "USWebCoatedSWOP_icc.h"
#include "Rec709-Rec1886_icc.h"

#include "SipiImage.h"
#include "shttps/makeunique.h"

namespace Sipi {

    void icc_error_logger(cmsContext ContextID, cmsUInt32Number ErrorCode, const char *Text) {
        std::cerr << "ICC-CMS-ERROR: " << Text << std::endl;
    }

    SipiIcc::SipiIcc(const unsigned char *icc_buf, int icc_len) {
        cmsSetLogErrorHandler(icc_error_logger);
        if ((icc_profile = cmsOpenProfileFromMem(icc_buf, icc_len)) == nullptr) {
            std::cerr << "THROWING ERROR IN ICC" << std::endl;
            throw SipiError(__file__, __LINE__, "cmsOpenProfileFromMem failed");
        }
        unsigned int len = cmsGetProfileInfoASCII(icc_profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, nullptr, 0);
        auto buf = shttps::make_unique<char[]>(len);
        cmsGetProfileInfoASCII(icc_profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, buf.get(), len);
        if (strcmp(buf.get(), "sRGB IEC61966-2.1") == 0) {
            profile_type = icc_sRGB;
        }
        else if (strncmp(buf.get(), "AdobeRGB", 8) == 0) {
            profile_type = icc_AdobeRGB;
        }
        else {
            profile_type = icc_unknown;
        }
    }

    SipiIcc::SipiIcc(const SipiIcc &icc_p) {
        cmsSetLogErrorHandler(icc_error_logger);
        if (icc_p.icc_profile != nullptr) {
            cmsUInt32Number len = 0;
            cmsSaveProfileToMem(icc_p.icc_profile, nullptr, &len);
            auto buf = shttps::make_unique<char[]>(len);
            cmsSaveProfileToMem(icc_p.icc_profile, buf.get(), &len);
            icc_profile = cmsOpenProfileFromMem(buf.get(), len);

            if (icc_profile == nullptr) {
                throw SipiError(__file__, __LINE__, "cmsOpenProfileFromMem failed");
            }

            profile_type = icc_p.profile_type;
        }
        else {
            icc_profile = nullptr;
            profile_type = icc_undefined;
        }
    }

    SipiIcc::SipiIcc(cmsHPROFILE &icc_profile_p) {
        cmsSetLogErrorHandler(icc_error_logger);
        if (icc_profile_p != nullptr) {
            cmsUInt32Number len = 0;
            cmsSaveProfileToMem(icc_profile_p, nullptr, &len);
            auto buf = shttps::make_unique<char[]>(len);
            cmsSaveProfileToMem(icc_profile_p, buf.get(), &len);
            if ((icc_profile = cmsOpenProfileFromMem(buf.get(), len)) == nullptr) {
                throw SipiError(__file__, __LINE__, "cmsOpenProfileFromMem failed");
            }
        }
        profile_type = icc_unknown;
    }

    SipiIcc::SipiIcc(PredefinedProfiles predef) {
        cmsSetLogErrorHandler(icc_error_logger);
        switch (predef) {
            case icc_undefined: {
                icc_profile = nullptr;
                profile_type = icc_undefined;
            }
            case icc_unknown: {
                throw SipiError(__file__, __LINE__, "Profile type \"icc_inknown\" not allowed");
            }
            case icc_sRGB: {
                icc_profile = cmsCreate_sRGBProfile();
                profile_type = icc_sRGB;
                break;
            }
            case icc_AdobeRGB: {
                icc_profile = cmsOpenProfileFromMem(AdobeRGB1998_icc, AdobeRGB1998_icc_len);
                profile_type = icc_AdobeRGB;
                break;
            }
            case icc_RGB: {
                throw SipiError(__file__, __LINE__, "Profile type \"icc_RGB\" uses other constructor");
            }
            case icc_CYMK_standard: {
                icc_profile = cmsOpenProfileFromMem(USWebCoatedSWOP_icc, USWebCoatedSWOP_icc_len);
                profile_type = icc_CYMK_standard;
                break;
            }
            case icc_GRAY_D50: {
                cmsContext context = cmsCreateContext(0, 0);
                icc_profile = cmsCreateGrayProfileTHR(context, cmsD50_xyY(), cmsBuildGamma(context, 2.2));
                profile_type = icc_GRAY_D50;
                break;
            }
            case icc_LUM_D65: {
                cmsContext context = cmsCreateContext(0, 0);
                icc_profile = cmsCreateGrayProfileTHR(context, cmsD50_xyY(), cmsBuildGamma(context, 2.4));
                profile_type = icc_LUM_D65;
                break;
            }
            case icc_ROMM_GRAY: {
                cmsContext context = cmsCreateContext(nullptr, nullptr);
                if (context == nullptr) std::cerr << "1 +++++++++++++++++++++++" << std::endl;
                cmsToneCurve* gaga = cmsBuildGamma(context, 1.8);
                if (gaga == nullptr) std::cerr << "2 +++++++++++++++++++++++" << std::endl;
                icc_profile = cmsCreateGrayProfileTHR(context, cmsD50_xyY(), gaga);
                if (icc_profile == nullptr) std::cerr << "3 +++++++++++++++++++++++" << std::endl;
                cmsDeleteContext(context);
                profile_type = icc_ROMM_GRAY;
                break;
            }
        }
    }

    SipiIcc::SipiIcc(float white_point_p[], float primaries_p[], const unsigned short tfunc[], const int tfunc_len) {
        cmsSetLogErrorHandler(icc_error_logger);
        cmsCIExyY white_point;
        white_point.x = white_point_p[0];
        white_point.y = white_point_p[1];
        white_point.Y = 1.0;

        cmsCIExyYTRIPLE primaries;
        primaries.Red.x = primaries_p[0];
        primaries.Red.y = primaries_p[1];
        primaries.Red.Y = 1.0;
        primaries.Green.x = primaries_p[2];
        primaries.Green.y = primaries_p[3];
        primaries.Green.Y = 1.0;
        primaries.Blue.x = primaries_p[4];
        primaries.Blue.y = primaries_p[5];
        primaries.Blue.Y = 1.0;

        cmsContext context = cmsCreateContext(0, 0);
        cmsToneCurve *tonecurve[3];
        if (tfunc == nullptr) {
            tonecurve[0] = cmsBuildGamma(context, 2.2);
            tonecurve[1] = cmsBuildGamma(context, 2.2);
            tonecurve[2] = cmsBuildGamma(context, 2.2);
        }
        else {
            tonecurve[0] = cmsBuildTabulatedToneCurve16(context, tfunc_len, tfunc);
            tonecurve[1] = cmsBuildTabulatedToneCurve16(context, tfunc_len, tfunc + tfunc_len);
            tonecurve[2] = cmsBuildTabulatedToneCurve16(context, tfunc_len, tfunc + 2*tfunc_len);
        }

        icc_profile = cmsCreateRGBProfileTHR(context, &white_point, &primaries, tonecurve);
        profile_type = icc_RGB;
        cmsFreeToneCurveTriple(tonecurve);
    }

    SipiIcc::~SipiIcc() {
        if (icc_profile != nullptr) {
            cmsCloseProfile(icc_profile);
        }
    }

    SipiIcc& SipiIcc::operator=(const SipiIcc &rhs) {
        if (this != &rhs) {
            if (rhs.icc_profile != nullptr) {
                unsigned int len = 0;
                cmsSaveProfileToMem(rhs.icc_profile, nullptr, &len);
                auto buf = shttps::make_unique<char[]>(len);
                cmsSaveProfileToMem(rhs.icc_profile, buf.get(), &len);
                if ((icc_profile = cmsOpenProfileFromMem(buf.get(), len)) == nullptr) {
                    throw SipiError(__file__, __LINE__, "cmsOpenProfileFromMem failed");
                }
            }
            profile_type = rhs.profile_type;
        }
        return *this;
    }

    unsigned char *SipiIcc::iccBytes(unsigned int &len) {
        unsigned char *buf = nullptr;
        len = 0;
        if (icc_profile != nullptr) {
            if (!cmsSaveProfileToMem(icc_profile, nullptr, &len)) throw SipiError(__file__, __LINE__, "cmsSaveProfileToMem failed");
            buf = new unsigned char[len];
            if (!cmsSaveProfileToMem(icc_profile, buf, &len)) throw SipiError(__file__, __LINE__, "cmsSaveProfileToMem failed");
        }
        return buf;
    }

    cmsHPROFILE SipiIcc::getIccProfile()  const {
        return icc_profile;
    }

    unsigned int SipiIcc::iccFormatter(int bps) const {
        cmsSetLogErrorHandler(icc_error_logger);
        cmsUInt32Number format = (bps == 16) ? BYTES_SH(2) : BYTES_SH(1);
        cmsColorSpaceSignature csig = cmsGetColorSpace(icc_profile);

        switch (csig) {
            case cmsSigLabData: {
                format |= CHANNELS_SH(3) | PLANAR_SH(0) | COLORSPACE_SH(PT_Lab);
                break;
            }
            case cmsSigYCbCrData: {
                format |= CHANNELS_SH(3) | PLANAR_SH(0) | COLORSPACE_SH(PT_YCbCr);
                break;
            }
            case cmsSigRgbData: {
                format |= CHANNELS_SH(3) | PLANAR_SH(0) | COLORSPACE_SH(PT_RGB);
                break;
            }
            case cmsSigGrayData: {
                format |= CHANNELS_SH(1) | PLANAR_SH(0) | COLORSPACE_SH(PT_GRAY);
                break;
            }
            case cmsSigCmykData: {
                format |= CHANNELS_SH(4) | PLANAR_SH(0) | COLORSPACE_SH(PT_CMYK);
                break;
            }
            default: {
                throw SipiError(__file__, __LINE__, "Unsupported iccFormatter for given profile");
            }
        }
        return format;
    }


    unsigned int SipiIcc::iccFormatter(SipiImage *img)  const {
        cmsSetLogErrorHandler(icc_error_logger);
        cmsUInt32Number format = 0;
        switch (img->bps) {
            case 8: {
                format = BYTES_SH(1) | CHANNELS_SH(img->nc) | PLANAR_SH(0);
                break;
            }
            case 16: {
                format = BYTES_SH(2) | CHANNELS_SH(img->nc) | PLANAR_SH(0);
                break;
            }
            default: {
                throw SipiError(__file__, __LINE__, "Unsupported bits/sample (" + std::to_string(img->bps) + ")");
            }
        }
        switch (img->photo) {
            case MINISWHITE: {
                format |= COLORSPACE_SH(PT_GRAY);
                break;
            }
            case MINISBLACK: {
                format |= COLORSPACE_SH(PT_GRAY);
                break;
            }
            case RGB: {
                format |= COLORSPACE_SH(PT_RGB);
                break;
            }
            case PALETTE: {
                throw SipiError(__file__, __LINE__, "Photometric interpretation \"PALETTE\" not supported");
                break;
            }
            case MASK: {
                throw SipiError(__file__, __LINE__, "Photometric interpretation \"MASK\" not supported");
                break;
            }
            case SEPARATED: { // --> CMYK
                format |= COLORSPACE_SH(PT_CMYK);
                break;
            }
            case YCBCR: {
                format |= COLORSPACE_SH(PT_YCbCr);
                break;
            }
            case CIELAB: {
                format |= COLORSPACE_SH(PT_Lab);
                break;
            }
            case ICCLAB: {
                format |= COLORSPACE_SH(PT_Lab);
                break;
            }
            case ITULAB: {
                format |= COLORSPACE_SH(PT_Lab);
                break;
            }
            case CFA: {
                throw SipiError(__file__, __LINE__, "Photometric interpretation \"Color Field Array (CFS)\" not supported");
                break;
            }
            case LOGL: {
                throw SipiError(__file__, __LINE__, "Photometric interpretation \"LOGL\" not supported");
                break;
            }
            case LOGLUV: {
                throw SipiError(__file__, __LINE__, "Photometric interpretation \"LOGLUV\" not supported");
                break;
            }
            case LINEARRAW: {
                throw SipiError(__file__, __LINE__, "Photometric interpretation \"LINEARRAW\" not supported");
                break;
            }
            default: {
                throw SipiError(__file__, __LINE__, "Photometric interpretation \"unknown\" not supported");
            }
        }
        return format;
    }

    std::ostream &operator<< (std::ostream &outstr, SipiIcc &rhs) {
        unsigned int len = cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, nullptr, 0);
        auto buf = shttps::make_unique<char[]>(len);
        cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, buf.get(), len);
        outstr << "ICC-Description : " << buf.get() << std::endl;

        len = cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoManufacturer, cmsNoLanguage, cmsNoCountry, nullptr, 0);
        buf = shttps::make_unique<char[]>(len);
        cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoManufacturer, cmsNoLanguage, cmsNoCountry, buf.get(), len);
        outstr << "ICC-Manufacturer: " << buf.get() << std::endl;

        len = cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoModel, cmsNoLanguage, cmsNoCountry, nullptr, 0);
        buf = shttps::make_unique<char[]>(len);
        cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoModel, cmsNoLanguage, cmsNoCountry, buf.get(), len);
        outstr << "ICC-Model       : " << buf.get() << std::endl;

        len = cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoCopyright, cmsNoLanguage, cmsNoCountry, nullptr, 0);
        buf = shttps::make_unique<char[]>(len);
        cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoCopyright, cmsNoLanguage, cmsNoCountry, buf.get(), len);
        outstr << "ICC-Copyright   : " << buf.get() << std::endl;

        struct tm datetime;
        if (cmsGetHeaderCreationDateTime(rhs.icc_profile, &datetime)) {
            outstr << "ICC-Date    : " << asctime(&datetime) << std::endl;
        }
        return outstr;
    }

}
